"""
model/plot_results.py

Reads model/sim_results.json and produces multiple figures in model/plots/:
  - median_latency.png
  - throughput.png
  - queue_arrival_*.png, queue_exec_*.png
  - queue_combined_*.png (arrival vs exec on same plot)
  - rates_*.png (enqueue vs dequeue rate over time)
  - utilization.png

Usage:
    python model/plot_results.py
"""
import json
import os
import matplotlib.pyplot as plt
import numpy as np

RESULTS_JSON_PATH = 'model/sim_results.json'
PLOTS_DIR = 'model/plots'
os.makedirs(PLOTS_DIR, exist_ok=True)


def load_results():
    with open(RESULTS_JSON_PATH, 'r') as f:
        return json.load(f)


def safe_get(d, *keys, default=None):
    try:
        for k in keys:
            d = d[k]
        return d
    except Exception:
        return default


def _resample_step(times_depths, dt=1.0):
    """Given a list of (t, depth) samples (not necessarily regular),
    produce step-sampled arrays (times, depths) at integer multiples of dt from 0..tmax.
    Uses forward-fill semantics so the step plot is stable between events."""
    if not times_depths:
        return [], []
    # sort by time
    times_depths = sorted(times_depths, key=lambda x: x[0])
    tmax = int(np.ceil(times_depths[-1][0]))
    if tmax <= 0:
        tmax = 1
    grid = np.arange(0, tmax + 1, dt)
    depths = np.zeros_like(grid, dtype=float)
    # iterate through events, fill forward
    idx = 0
    cur_depth = 0
    ev_iter = iter(times_depths)
    next_ev = next(ev_iter, None)
    for i, g in enumerate(grid):
        # consume events up to grid time
        while next_ev is not None and next_ev[0] <= g:
            cur_depth = next_ev[1]
            next_ev = next(ev_iter, None)
        depths[i] = cur_depth
    return grid, depths


def _moving_average(x, window=5):
    if len(x) == 0:
        return x
    window = max(1, int(window))
    kernel = np.ones(window) / window
    return np.convolve(x, kernel, mode='same')


def plot_median_latency(sw, hw):
    labels = ['SW (baseline)', 'HW (leader queue)']
    sw_med = safe_get(sw, 'latency_stats', 'median_ns', default=0)
    hw_med = safe_get(hw, 'latency_stats', 'median_ns', default=0)
    vals = [sw_med, hw_med]

    plt.figure(figsize=(8, 5))
    bars = plt.bar(labels, vals, color=['#d9534f', '#5cb85c'])
    plt.yscale('log')
    plt.ylabel('Median Task Latency (ns)')
    plt.title('Median Task Latency: SW vs HW')
    if sw_med and hw_med:
        speedup = sw_med / max(1e-9, hw_med)
        plt.text(0.5, max(vals) * 1.05, f"{speedup:.2f}x median speedup", ha='center')
    for b in bars:
        y = b.get_height()
        plt.text(b.get_x() + b.get_width() / 2, y, f"{int(y)} ns", va='bottom', ha='center')
    plt.tight_layout()
    plt.savefig(os.path.join(PLOTS_DIR, 'median_latency.png'))
    plt.close()


def plot_throughput(sw, hw):
    sw_time = safe_get(sw, 'summary', 'env_now', default=0)
    hw_time = safe_get(hw, 'summary', 'env_now', default=0)
    sw_done = safe_get(sw, 'completed_tasks', default=0)
    hw_done = safe_get(hw, 'completed_tasks', default=0)

    sw_thr = sw_done / max(1e-9, sw_time)
    hw_thr = hw_done / max(1e-9, hw_time)

    plt.figure(figsize=(8, 5))
    plt.bar(['SW', 'HW'], [sw_thr, hw_thr], color=['#6c5ce7', '#00b894'])
    plt.ylabel('Throughput (tasks / ns)')
    plt.title('Throughput Comparison')
    plt.tight_layout()
    plt.savefig(os.path.join(PLOTS_DIR, 'throughput.png'))
    plt.close()


def plot_queue_series(obj, tag):
    # Convert JSON samples into (t,d) lists
    arr = safe_get(obj, 'queue_ts_arrival', default=[])
    ex = safe_get(obj, 'queue_ts_exec', default=[])

    arr_td = [(p['t'], p['depth']) for p in arr] if arr else []
    ex_td = [(p['t'], p['depth']) for p in ex] if ex else []

    # Choose dt based on the density of samples (1 ns or  dt ~ 1)
    dt = 1.0

    # Arrival buffer
    if arr_td:
        t_arr, d_arr = _resample_step(arr_td, dt=dt)
        plt.figure(figsize=(10, 4))
        plt.step(t_arr, d_arr, where='post')
        plt.xlabel('Time (ns)')
        plt.ylabel('Arrival Buffer Depth')
        plt.title(f'Arrival Buffer Depth over time ({tag})')
        plt.tight_layout()
        plt.savefig(os.path.join(PLOTS_DIR, f'queue_arrival_{tag}.png'))
        plt.close()

    # Exec buffer
    if ex_td:
        t_ex, d_ex = _resample_step(ex_td, dt=dt)
        plt.figure(figsize=(10, 4))
        plt.step(t_ex, d_ex, where='post')
        plt.xlabel('Time (ns)')
        plt.ylabel('Execution Queue Depth')
        plt.title(f'Execution Queue Depth over time ({tag})')
        plt.tight_layout()
        plt.savefig(os.path.join(PLOTS_DIR, f'queue_exec_{tag}.png'))
        plt.close()
    else:
        # produce an explicit zero-line plot (same x-range as arrival if possible)
        if arr_td:
            t_arr, _ = _resample_step(arr_td, dt=dt)
            zero_depth = np.zeros_like(t_arr)
            plt.figure(figsize=(10, 4))
            plt.step(t_arr, zero_depth, where='post')
            plt.xlabel('Time (ns)')
            plt.ylabel('Execution Queue Depth')
            plt.title(f'Execution Queue Depth over time ({tag})')
            plt.tight_layout()
            plt.savefig(os.path.join(PLOTS_DIR, f'queue_exec_{tag}.png'))
            plt.close()

    # Combined (arrival vs exec)
    if arr_td or ex_td:
        # determine unified time grid
        all_td = arr_td + ex_td
        if not all_td:
            return
        t_unified, _ = _resample_step(all_td, dt=dt)
        plt.figure(figsize=(10, 4))
        if arr_td:
            t_arr, d_arr = _resample_step(arr_td, dt=dt)
            plt.step(t_arr, d_arr, where='post', label='Arrival buffer')
        if ex_td:
            t_ex, d_ex = _resample_step(ex_td, dt=dt)
            plt.step(t_ex, d_ex, where='post', label='Exec queue')
        else:
            plt.step(t_unified, np.zeros_like(t_unified), where='post', label='Exec queue (0)')
        plt.xlabel('Time (ns)'); plt.ylabel('Queue Depth')
        plt.title(f'Arrival vs Exec Queue Depth ({tag})')
        plt.legend(); plt.tight_layout(); plt.savefig(os.path.join(PLOTS_DIR, f'queue_combined_{tag}.png')); plt.close()


def plot_rates(obj, tag):
    rs = safe_get(obj, 'rate_ts', default=[])
    if not rs:
        return
    t = np.array([p['t'] for p in rs])
    enq = np.array([p['enq'] for p in rs])
    deq = np.array([p['deq'] for p in rs])

    # smooth the noisy instantaneous rates with a small moving average
    window = max(1, int(len(t) / 50))  # small relative smoothing
    enq_s = _moving_average(enq, window=window)
    deq_s = _moving_average(deq, window=window)

    plt.figure(figsize=(10, 4))
    plt.plot(t, enq_s, label='Enqueue rate (tasks/ns)')
    plt.plot(t, deq_s, label='Dequeue rate (tasks/ns)')
    plt.xlabel('Time (ns)'); plt.ylabel('Rate (tasks/ns)')
    plt.title(f'Enqueue vs Dequeue Rate ({tag})')
    plt.legend(); plt.tight_layout()
    plt.savefig(os.path.join(PLOTS_DIR, f'rates_{tag}.png'))
    plt.close()


def plot_utilization(sw, hw):
    l_sw = safe_get(sw, 'leader_util', default={})
    l_hw = safe_get(hw, 'leader_util', default={})
    fb_sw = safe_get(sw, 'follower_util_sample', default={})
    fb_hw = safe_get(hw, 'follower_util_sample', default={})

    avg_l_sw = sum(l_sw.values()) / len(l_sw) if l_sw else 0
    avg_l_hw = sum(l_hw.values()) / len(l_hw) if l_hw else 0
    avg_f_sw = sum(fb_sw.values()) / len(fb_sw) if fb_sw else 0
    avg_f_hw = sum(fb_hw.values()) / len(fb_hw) if fb_hw else 0

    plt.figure(figsize=(10, 5))
    plt.bar(['Leader SW', 'Leader HW', 'Follower SW (sample)', 'Follower HW (sample)'],
            [avg_l_sw, avg_l_hw, avg_f_sw, avg_f_hw],
            color=['#fd79a8', '#00b894', '#74b9ff', '#0984e3'])
    plt.ylabel('Utilization (fraction busy)')
    plt.title('Leader and Follower Utilization (averages)')
    plt.xticks(rotation=15)
    plt.tight_layout(); plt.savefig(os.path.join(PLOTS_DIR, 'utilization.png'))
    plt.close()


def main():
    try:
        results = load_results()
    except FileNotFoundError:
        print(f"Results not found at {RESULTS_JSON_PATH}. Run behavioral.py first.")
        return

    sw = results.get('software', {})
    hw = results.get('hardware', {})

    plot_median_latency(sw, hw)
    plot_throughput(sw, hw)
    plot_queue_series(sw, 'sw')
    plot_queue_series(hw, 'hw')
    plot_rates(sw, 'sw')
    plot_rates(hw, 'hw')
    plot_utilization(sw, hw)
    print(f"All plots saved to {PLOTS_DIR}")


if __name__ == '__main__':
    main()
