"""
model/behavioral.py

Behavioral simulation for Leader/Follower dispatching (HammerBlade control-accelerator model).

Produces a JSON file at `model/sim_results.json` containing both SW and HW experiment results.

Usage (ad-hoc):
    python model/behavioral.py

Configuration: tune the global parameters near the top of the file or supply a
`config` dict to `run_sim()` for automated sweeps.

Units: all times are in nanoseconds (ns).
"""
import simpy
import random
import json
import os
import statistics
from collections import defaultdict

# ----------------------------
# Tunable parameters (override here or via config dict in run_sim)
# ----------------------------
SEED = 42

NUM_TASKS = 1000
NUM_FOLLOWERS = 64
NUM_LEADERS = 1

TASK_DURATION_MEAN = 10.0   # ns
TASK_DURATION_SD = 0.0      # ns stddev

DISPATCH_TIME_SW = 5.0      # ns per dispatch (software path)
DISPATCH_TIME_HW = 1.0      # ns per dispatch (hardware path)

ARRIVAL_RATE = 0.2          # tasks/ns (Poisson). Use moderate values unless stress-testing
ARRIVAL_MODEL = 'poisson'   # 'poisson' or 'deterministic'

EXEC_QUEUE_CAPACITY = None  # Not used (Store is unbounded in this simple model)
BATCH_ENQUEUE = 1           # tasks enqueued per dispatch by a leader (1 = no burst)

SIM_TRACE_SAMPLE_INTERVAL = 2.0  # ns between samples for depth and rate

OUTPUT_FILE = "model/sim_results.json"

# ----------------------------
# Metrics
# ----------------------------
class Metrics:
    def __init__(self):
        self.task_info = {}  # tid -> timestamps
        self.queue_samples_exec = []    # (t, depth)
        self.queue_samples_arrival = [] # (t, depth)
        self.rate_samples = []          # (t, enq_rate, deq_rate)

        self.leader_busy_time = defaultdict(float)   # dispatch + put blocking
        self.leader_block_time = defaultdict(float)  # time blocked on full exec queue
        self.follower_busy_time = defaultdict(float)

        self.total_enqueued = 0
        self.total_dequeued = 0
        self.completed_tasks = 0

    # task events
    def record_arrival(self, tid, t):
        self.task_info[tid] = {"arrival": t}

    def record_enqueued(self, tid, t, leader_id):
        d = self.task_info.setdefault(tid, {})
        d["enqueued"] = t
        d["enqueued_by"] = leader_id

    def record_start_exec(self, tid, t, follower_id):
        d = self.task_info.setdefault(tid, {})
        d["start_exec"] = t
        d["follower_id"] = follower_id

    def record_finish(self, tid, t):
        d = self.task_info.setdefault(tid, {})
        d["finish"] = t
        self.completed_tasks += 1

    # sampling
    def sample_exec(self, t, depth):
        self.queue_samples_exec.append((t, depth))

    def sample_arrival(self, t, depth):
        self.queue_samples_arrival.append((t, depth))

    def sample_rates(self, t, enq_rate, deq_rate):
        self.rate_samples.append((t, enq_rate, deq_rate))


# ----------------------------
# Processes
# ----------------------------
def arrival_process(env, metrics, arrival_count, interarrival_func, arrival_buffer):
    """Create tasks and put them into the arrival buffer."""
    for i in range(arrival_count):
        tid = f"T{i}"
        metrics.record_arrival(tid, env.now)
        # Put into arrival buffer
        yield arrival_buffer.put(tid)
        # Wait interarrival time
        ia = interarrival_func()
        if ia > 0:
            yield env.timeout(ia)


def leader_process(env, leader_id, metrics, arrival_buffer, exec_queue, dispatch_time_func):
    """Leader pulls from arrival buffer, pays dispatch time once per batch, then enqueues BATCH_ENQUEUE tasks.
    If exec queue is bounded and full, the leader accrues block time while waiting to put()."""
    while True:
        # if there are no tasks left and all tasks have been enqueued, block here - run_sim will exit when all finished
        tid = yield arrival_buffer.get()  # blocks until available
        batch = [tid]

        # Try to build a batch from immediately available arrivals (non-blocking)
        # We directly consume arrival_buffer.items to form a batch of ready tasks
        while len(batch) < BATCH_ENQUEUE and arrival_buffer.items:
            # pop from left
            batch.append(arrival_buffer.items.pop(0))

        # Dispatch cost once per batch
        dt = dispatch_time_func()
        start = env.now
        if dt > 0:
            yield env.timeout(dt)
        dispatch_elapsed = env.now - start
        metrics.leader_busy_time[leader_id] += dispatch_elapsed

        # Enqueue the batch; this will block if exec_queue.get() consumers are busy
        for t in batch:
            before_put = env.now
            yield exec_queue.put(t)
            put_elapsed = env.now - before_put
            if put_elapsed > 0:
                metrics.leader_block_time[leader_id] += put_elapsed
            metrics.leader_busy_time[leader_id] += put_elapsed
            metrics.total_enqueued += 1
            metrics.record_enqueued(t, env.now, leader_id)


def follower_worker(env, fid, metrics, exec_queue, task_duration_func):
    """Followers continuously take tasks from exec_queue and execute them (simulate CPU cores)."""
    while True:
        tid = yield exec_queue.get()
        # Mark start of execution
        metrics.record_start_exec(tid, env.now, fid)
        td = task_duration_func()
        start = env.now
        if td > 0:
            yield env.timeout(td)
        metrics.follower_busy_time[fid] += (env.now - start)
        metrics.record_finish(tid, env.now)
        metrics.total_dequeued += 1


def depth_sampler(env, metrics, exec_queue, arrival_buffer, interval):
    """Sample queue depths at regular intervals (forward-fill friendly)."""
    while True:
        metrics.sample_exec(env.now, len(exec_queue.items))
        metrics.sample_arrival(env.now, len(arrival_buffer.items))
        yield env.timeout(interval)


def rate_sampler(env, metrics, interval):
    """Sample enqueue/dequeue rates using differences in cumulative counters."""
    last_t = env.now
    last_enq = metrics.total_enqueued
    last_deq = metrics.total_dequeued
    while True:
        yield env.timeout(interval)
        now = env.now
        dt = max(1e-9, now - last_t)
        enq_rate = (metrics.total_enqueued - last_enq) / dt
        deq_rate = (metrics.total_dequeued - last_deq) / dt
        metrics.sample_rates(now, enq_rate, deq_rate)
        last_t, last_enq, last_deq = now, metrics.total_enqueued, metrics.total_dequeued


# ----------------------------
# Distributions
# ----------------------------
def make_exponential(rate):
    if rate <= 0:
        return lambda: 0.0
    return lambda: random.expovariate(rate)


def make_deterministic(value):
    return lambda: value


def make_normal(mu, sigma):
    if sigma <= 0:
        return lambda: mu
    return lambda: max(0.0, random.gauss(mu, sigma))


# ----------------------------
# Runner
# ----------------------------
def run_sim(mode="hw", config=None, verbose=False):
    global NUM_TASKS, NUM_FOLLOWERS, NUM_LEADERS
    global TASK_DURATION_MEAN, TASK_DURATION_SD
    global DISPATCH_TIME_SW, DISPATCH_TIME_HW
    global ARRIVAL_RATE, ARRIVAL_MODEL, EXEC_QUEUE_CAPACITY, BATCH_ENQUEUE
    global SIM_TRACE_SAMPLE_INTERVAL

    if config:
        for k, v in config.items():
            if k in globals():
                globals()[k] = v

    random.seed(SEED)
    env = simpy.Environment()
    metrics = Metrics()

    # Queues (simple Store-based queue; unbounded for this behavioral model)
    arrival_buffer = simpy.Store(env)   # producer -> arrival_buffer.put()
    exec_queue = simpy.Store(env)       # leaders -> exec_queue.put(); followers -> exec_queue.get()

    # Distributions
    if ARRIVAL_MODEL == 'poisson':
        interarrival = make_exponential(ARRIVAL_RATE)
    else:
        interarrival = make_deterministic(1.0 / ARRIVAL_RATE if ARRIVAL_RATE > 0 else 0.0)

    task_duration_fn = make_normal(TASK_DURATION_MEAN, TASK_DURATION_SD)
    dispatch_time = DISPATCH_TIME_SW if mode == 'sw' else DISPATCH_TIME_HW
    dispatch_fn = make_deterministic(dispatch_time)

    # Back-of-the-envelope rates (printed once for sanity)
    follower_capacity = NUM_FOLLOWERS / max(1e-9, TASK_DURATION_MEAN)
    leader_rate = (NUM_LEADERS * BATCH_ENQUEUE) / max(1e-9, dispatch_time)
    if verbose:
        print(f"[mode={mode}] follower capacity ≈ {follower_capacity:.3f} tasks/ns | leader rate ≈ {leader_rate:.3f} tasks/ns | arrival {ARRIVAL_RATE:.3f}")

    # Start processes
    env.process(arrival_process(env, metrics, NUM_TASKS, interarrival, arrival_buffer))
    for lid in range(NUM_LEADERS):
        env.process(leader_process(env, lid, metrics, arrival_buffer, exec_queue, dispatch_fn))
    for fid in range(NUM_FOLLOWERS):
        env.process(follower_worker(env, fid, metrics, exec_queue, task_duration_fn))
    env.process(depth_sampler(env, metrics, exec_queue, arrival_buffer, SIM_TRACE_SAMPLE_INTERVAL))
    env.process(rate_sampler(env, metrics, SIM_TRACE_SAMPLE_INTERVAL))

    # Run until all tasks done or cap
    MAX_SIM_TIME = 1e9
    while True:
        if metrics.completed_tasks >= NUM_TASKS:
            break
        if env.now > MAX_SIM_TIME:
            print("Max sim time reached; aborting.")
            break
        env.step()

    # Summaries
    latencies, queue_waits = [], []
    for tid, data in metrics.task_info.items():
        a = data.get("arrival")
        s = data.get("start_exec")
        f = data.get("finish")
        q = data.get("enqueued")
        if a is not None and f is not None:
            latencies.append(f - a)
        if q is not None and s is not None:
            queue_waits.append(s - q)

    def quantiles_safe(xs, n, idx_default):
        if len(xs) >= n:
            return statistics.quantiles(xs, n=n)[idx_default]
        return max(xs) if xs else 0

    latency_stats = {
        "avg_ns": statistics.mean(latencies) if latencies else 0,
        "median_ns": statistics.median(latencies) if latencies else 0,
        "p90_ns": quantiles_safe(latencies, 10, 8),
        "p99_ns": quantiles_safe(latencies, 100, 98),
        "min_ns": min(latencies) if latencies else 0,
        "max_ns": max(latencies) if latencies else 0,
    }

    wait_stats = {
        "avg_wait_ns": statistics.mean(queue_waits) if queue_waits else 0,
        "median_wait_ns": statistics.median(queue_waits) if queue_waits else 0,
        "max_wait_ns": max(queue_waits) if queue_waits else 0,
    }

    total_time = max(1.0, env.now)
    leader_util = {k: v / total_time for k, v in metrics.leader_busy_time.items()}
    leader_block = {k: v / total_time for k, v in metrics.leader_block_time.items()}
    follower_util = {k: v / total_time for k, v in metrics.follower_busy_time.items()}

    results = {
        "summary": {
            "mode": mode,
            "env_now": env.now,
            "num_tasks": NUM_TASKS,
            "num_followers": NUM_FOLLOWERS,
            "num_leaders": NUM_LEADERS,
            "task_duration_mean": TASK_DURATION_MEAN,
            "dispatch_time": dispatch_time,
            "arrival_rate": ARRIVAL_RATE,
            "queue_capacity": EXEC_QUEUE_CAPACITY,
            "batch_enqueue": BATCH_ENQUEUE,
        },
        "latency_stats": latency_stats,
        "wait_stats": wait_stats,
        "leader_util": leader_util,
        "leader_block_fraction": leader_block,
        "follower_util_sample": {k: follower_util[k] for k in list(follower_util)[:min(16, len(follower_util))]},
        "queue_ts_exec": [{"t": float(t), "depth": int(d)} for (t, d) in metrics.queue_samples_exec],
        "queue_ts_arrival": [{"t": float(t), "depth": int(d)} for (t, d) in metrics.queue_samples_arrival],
        "rate_ts": [{"t": float(t), "enq": float(e), "deq": float(d)} for (t, e, d) in metrics.rate_samples],
        "completed_tasks": metrics.completed_tasks,
        "total_enqueued": metrics.total_enqueued,
        "total_dequeued": metrics.total_dequeued,
    }
    return results


if __name__ == "__main__":
    random.seed(SEED)
    print("Running software dispatch (SW) experiment...")
    sw = run_sim(mode="sw", verbose=True)
    print("Running hardware dispatch (HW) experiment...")
    hw = run_sim(mode="hw", verbose=True)

    os.makedirs(os.path.dirname(OUTPUT_FILE) or ".", exist_ok=True)
    with open(OUTPUT_FILE, "w") as f:
        json.dump({"software": sw, "hardware": hw}, f, indent=4)
    print(f"Saved results to {OUTPUT_FILE}")
