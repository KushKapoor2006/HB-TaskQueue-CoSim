#!/usr/bin/env python3
"""
model/run_ablation.py

Driver to run ablation experiments against the behavioral simulation.

Features:
- Defines a default ablation matrix (several named experiments).
- Optionally reads an external YAML config with experiment specs (path passed with --config).
- Runs each experiment for multiple random seeds.
- Tries to import model.behavioral.run_sim() and call it directly (preferred).
  If import fails, falls back to calling `python model/behavioral.py` as a subprocess.
- Writes per-run JSON results to model/results/<experiment>__seed_<s>.json
- Appends a summary row to model/results/ablation_results.csv for each run.

Usage:
    python model/run_ablation.py
    python model/run_ablation.py --config my_ablation.yaml --seeds 42 99 123

Requirements:
- If you want YAML config support, install pyyaml: pip install pyyaml
"""
from __future__ import annotations
import argparse
import json
import os
import csv
import time
import subprocess
import importlib
from typing import Any, Dict, List

# Optional: yaml for config file. If not available we proceed using defaults.
try:
    import yaml  # type: ignore
    YAML_AVAILABLE = True
except Exception:
    YAML_AVAILABLE = False

# locations
BASE_DIR = os.path.dirname(os.path.dirname(__file__)) or "."
MODEL_RESULTS_DIR = os.path.join(BASE_DIR, "model", "results")
os.makedirs(MODEL_RESULTS_DIR, exist_ok=True)

# defaults
DEFAULT_SEEDS = [42, 100, 2025]
DEFAULT_NUM_REPEATS = 1
DEFAULT_EXPERIMENTS = [
    {
        "name": "baseline_sw",
        "mode": "sw",
        # keep other params None to use behavioral.py defaults
    },
    {
        "name": "baseline_hw",
        "mode": "hw",
    },
    # Vary number of followers (test follower bottleneck)
    {
        "name": "followers_4",
        "mode": "hw",
        "NUM_FOLLOWERS": 4,
        "TASK_DURATION_MEAN": 50.0,
        "ARRIVAL_RATE": 1.0
    },
    {
        "name": "followers_16",
        "mode": "hw",
        "NUM_FOLLOWERS": 16,
        "TASK_DURATION_MEAN": 20.0,
        "ARRIVAL_RATE": 0.6
    },
    # Leader density sweep
    {
        "name": "leaders_2_batch4",
        "mode": "hw",
        "NUM_LEADERS": 2,
        "BATCH_ENQUEUE": 4,
        "ARRIVAL_RATE": 1.0,
        "TASK_DURATION_MEAN": 10.0
    },
    # Slow software dispatch but fast arrivals (to show SW throughput collapse)
    {
        "name": "sw_arrival_heavy",
        "mode": "sw",
        "ARRIVAL_RATE": 0.6,
        "NUM_FOLLOWERS": 8,
        "TASK_DURATION_MEAN": 5.0
    }
]


def try_import_run_sim():
    """Try to import model.behavioral.run_sim. Returns callable or None."""
    try:
        mod = importlib.import_module("model.behavioral")
        if hasattr(mod, "run_sim"):
            return getattr(mod, "run_sim")
    except Exception:
        return None
    return None


def call_run_sim_direct(run_sim_func, mode: str, config: Dict[str, Any], verbose: bool = False) -> Dict[str, Any]:
    """
    Call the imported run_sim() function directly.
    run_sim signature expected: run_sim(mode='hw'|'sw', config=dict|None, verbose=False) -> dict (results)
    """
    # Defensive: ensure config is a dict
    cfg = config if isinstance(config, dict) else {}
    return run_sim_func(mode=mode, config=cfg, verbose=verbose)


def call_run_sim_subprocess(mode: str, config: Dict[str, Any], outfile_json: str, verbose: bool = False) -> Dict[str, Any]:
    """
    Fallback runner: invoke `python model/behavioral.py` with a small temporary config file.
    The behavioral script must write model/sim_results.json — we will read it and extract the requested mode.
    This function creates a temporary config JSON (env var style) and runs the behavioral script.
    """
    # create a small temporary config file (json) that behavioral.py can read if modified to do so.
    # Our behavioral.py implementation accepts a 'config' dict only through import. If you're using subprocess
    # fallback, we'll set some environment variables for behavioral.py to read — so behavioral.py must support it.
    # To keep this generic, we will run behavioral.py and then read model/sim_results.json and pick the relevant mode.
    import sys
    proc = subprocess.run([sys.executable, os.path.join("model", "behavioral.py")], capture_output=not verbose)

    if proc.returncode != 0:
        print("Subprocess run failed:")
        if proc.stdout:
            print(proc.stdout.decode())
        if proc.stderr:
            print(proc.stderr.decode())
        raise RuntimeError("behavioral.py subprocess failed")
    # read the generated JSON
    sim_results_path = os.path.join("model", "sim_results.json")
    if not os.path.exists(sim_results_path):
        # maybe the behavioral script wrote to model/results/sim_results.json
        sim_results_path = os.path.join("model", "results", "sim_results.json")
    with open(sim_results_path, "r") as f:
        data = json.load(f)
    # This returns both 'software' and 'hardware' keys if behavioral.py wrote both runs.
    return data


def experiment_runner(experiment: Dict[str, Any], seeds: List[int], outdir: str, run_sim_func):
    """
    Run a single experiment dict for all seeds.
    experiment: dictionary describing the experiment. Must contain 'name' and 'mode'.
    seeds: list of integer seeds (will be set as SEED in behavioral config)
    run_sim_func: callable or None. If None we will try subprocess fallback.
    """
    rows = []

    name = experiment.get("name", "exp")
    mode = experiment.get("mode", "hw")

    # figure out the parameter keys to pass into run_sim as config
    # exclude reserved keys
    reserved = {"name", "mode"}
    exp_config = {k: v for k, v in experiment.items() if k not in reserved}

    for seed in seeds:
        run_cfg = dict(exp_config)  # shallow copy
        run_cfg["SEED"] = seed  # some implementations may expect this in config

        print(f"\n[RUN] experiment={name} mode={mode} seed={seed} cfg={run_cfg}")

        if run_sim_func:
            try:
                results = call_run_sim_direct(run_sim_func, mode, run_cfg, verbose=False)
            except Exception as e:
                print(f"Direct call to run_sim failed: {e}. Falling back to subprocess.")
                results = call_run_sim_subprocess(mode, run_cfg, "", verbose=False)
        else:
            # fallback
            results = call_run_sim_subprocess(mode, run_cfg, "", verbose=False)

        # results is expected to be a dict with top-level keys 'software' and/or 'hardware'
        # If behavioral.run_sim was called directly it returns the dict for that run only.
        # Normalize: if the returned dict contains 'summary' -> treat as single-run results.
        if isinstance(results, dict) and "summary" in results:
            single = results
        else:
            # If behavioral script returns both modes, pick by mode key
            if isinstance(results, dict) and mode in results:
                single = results[mode]
            else:
                # Unexpected format
                single = results

        # Save per-run JSON
        out_json = os.path.join(outdir, f"{name}__seed_{seed}.json")
        with open(out_json, "w") as f:
            json.dump(single, f, indent=2)
        print(f"Wrote run JSON -> {out_json}")

        # Build CSV row from available fields (use safe getters)
        summary = single.get("summary", {})
        lat = single.get("latency_stats", {})
        leader_util_map = single.get("leader_util", {}) or {}
        follower_util_map = single.get("follower_util_sample", {}) or {}
        leader_block_map = single.get("leader_block_fraction", {}) or {}

        leader_util_avg = sum(leader_util_map.values()) / len(leader_util_map) if leader_util_map else 0.0
        follower_util_avg = sum(follower_util_map.values()) / len(follower_util_map) if follower_util_map else 0.0
        leader_block_avg = sum(leader_block_map.values()) / len(leader_block_map) if leader_block_map else 0.0

        row = {
            "experiment": name,
            "seed": seed,
            "mode": mode,
            "num_tasks": summary.get("num_tasks"),
            "num_followers": summary.get("num_followers"),
            "num_leaders": summary.get("num_leaders"),
            "batch_enqueue": summary.get("batch_enqueue"),
            "dispatch_time": summary.get("dispatch_time"),
            "arrival_rate": summary.get("arrival_rate"),
            "env_now": summary.get("env_now"),
            "completed_tasks": single.get("completed_tasks"),
            "median_ns": lat.get("median_ns"),
            "p90_ns": lat.get("p90_ns"),
            "p99_ns": lat.get("p99_ns"),
            "total_enqueued": single.get("total_enqueued"),
            "total_dequeued": single.get("total_dequeued"),
            "leader_util_avg": leader_util_avg,
            "leader_block_frac": leader_block_avg,
            "follower_util_avg_sample": follower_util_avg,
        }

        rows.append(row)

    return rows


def load_experiments_from_yaml(path: str):
    if not YAML_AVAILABLE:
        raise RuntimeError("YAML not available (pyyaml). Install with `pip install pyyaml` or use defaults.")
    with open(path, "r") as f:
        cfg = yaml.safe_load(f)
    # Expect cfg to be a list of experiment dicts or a dict with key 'experiments'
    if isinstance(cfg, dict) and "experiments" in cfg:
        return cfg["experiments"]
    if isinstance(cfg, list):
        return cfg
    raise ValueError("Unsupported YAML format for experiments.")


def write_csv_rows(csv_path: str, rows: List[Dict[str, Any]]):
    header = [
        "experiment", "seed", "mode", "num_tasks", "num_followers", "num_leaders", "batch_enqueue",
        "dispatch_time", "arrival_rate", "env_now", "completed_tasks",
        "median_ns", "p90_ns", "p99_ns",
        "total_enqueued", "total_dequeued",
        "leader_util_avg", "leader_block_frac", "follower_util_avg_sample"
    ]
    write_header = not os.path.exists(csv_path)
    with open(csv_path, "a", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=header)
        if write_header:
            writer.writeheader()
        for r in rows:
            # ensure all keys are present
            out = {k: r.get(k, None) for k in header}
            writer.writerow(out)


def main():
    parser = argparse.ArgumentParser(description="Run ablation experiments for the behavioral model")
    parser.add_argument("--config", type=str, default="", help="YAML file with experiments (list of dicts) to run")
    parser.add_argument("--seeds", type=int, nargs="*", default=DEFAULT_SEEDS, help="List of seeds to run")
    parser.add_argument("--outdir", type=str, default=MODEL_RESULTS_DIR, help="Directory to write per-run JSONs and CSV")
    parser.add_argument("--csv", type=str, default=os.path.join(MODEL_RESULTS_DIR, "ablation_results.csv"), help="CSV output path")
    parser.add_argument("--dry", action="store_true", help="Dry-run: print experiments but don't execute")
    args = parser.parse_args()

    if args.config:
        if not YAML_AVAILABLE:
            raise RuntimeError("PyYAML is required to load a YAML config. Install pyyaml or omit --config.")
        experiments = load_experiments_from_yaml(args.config)
    else:
        experiments = DEFAULT_EXPERIMENTS

    print(f"Using {len(experiments)} experiments, seeds={args.seeds}, output dir={args.outdir}")

    # try import
    run_sim_func = try_import_run_sim()
    if run_sim_func:
        print("Imported run_sim from model.behavioral (direct invocation mode).")
    else:
        print("Could not import model.behavioral.run_sim - will fall back to subprocess invocation if needed.")

    all_rows = []
    for exp in experiments:
        print(f"\n=== EXPERIMENT: {exp.get('name','unnamed')} ===")
        if args.dry:
            print("Dry run: would execute with config:", exp)
            continue
        rows = experiment_runner(exp, args.seeds, args.outdir, run_sim_func)
        all_rows.extend(rows)
        # after each experiment, flush rows to CSV to save progress
        write_csv_rows(args.csv, rows)
        print(f"Wrote {len(rows)} rows to {args.csv}")

    print("\nAll experiments finished.")


if __name__ == "__main__":
    main()
