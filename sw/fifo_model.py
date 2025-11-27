# model/fifo_model.py
#
# Minimal golden FIFO checker for HammerBlade task queue.
# Usage (from project root):
#   python3 model/fifo_model.py sw/logs/trace.csv
#
# It replays all "push,0xXXXXXXXX" and "pop,0xXXXXXXXX" events and checks:
#   - pops never happen on empty queue
#   - popped values match FIFO order of pushes
#
# On success: prints summary and exits 0
# On mismatch: prints details and exits 1

import sys
from collections import deque
import json
import os

def check_trace(trace_path: str):
    q = deque()
    mismatches = 0
    pushes = 0
    pops = 0

    with open(trace_path, "r") as f:
        header = f.readline()
        for lineno, line in enumerate(f, start=2):
            line = line.strip()
            if not line:
                continue
            parts = line.split(",")
            if len(parts) != 2:
                print(f"[PY] line {lineno}: malformed: {line}")
                continue
            op, val_str = parts
            op = op.strip().lower()
            val_str = val_str.strip()
            try:
                v = int(val_str, 16)
            except ValueError:
                print(f"[PY] line {lineno}: invalid hex value: {val_str}")
                mismatches += 1
                continue

            if op == "push":
                q.append(v)
                pushes += 1
            elif op == "pop":
                pops += 1
                if not q:
                    print(f"[PY] line {lineno}: POP on empty queue (value=0x{v:08x})")
                    mismatches += 1
                else:
                    expected = q.popleft()
                    if expected != v:
                        print(f"[PY] line {lineno}: MISMATCH: expected 0x{expected:08x}, got 0x{v:08x}")
                        mismatches += 1
            else:
                print(f"[PY] line {lineno}: unknown op '{op}'")
                mismatches += 1

    return {
        "trace_path": trace_path,
        "pushes": pushes,
        "pops": pops,
        "final_queue_len": len(q),
        "mismatches": mismatches,
    }

def main():
    if len(sys.argv) < 2:
        print("Usage: python3 model/fifo_model.py <trace.csv> [output_json]")
        sys.exit(1)

    trace_path = sys.argv[1]
    out_json = None
    if len(sys.argv) >= 3:
        out_json = sys.argv[2]
    else:
        # default: write JSON next to trace as golden_results.json
        base_dir = os.path.dirname(trace_path) or "."
        out_json = os.path.join(base_dir, "golden_results.json")

    res = check_trace(trace_path)

    print("=== Python FIFO golden check ===")
    print(f"Trace file       : {res['trace_path']}")
    print(f"Pushes (success) : {res['pushes']}")
    print(f"Pops   (success) : {res['pops']}")
    print(f"Final queue len  : {res['final_queue_len']}")
    print(f"Mismatches       : {res['mismatches']}")

    try:
        with open(out_json, "w") as f:
            json.dump(res, f, indent=2)
        print(f"[PY] wrote golden results to {out_json}")
    except OSError as e:
        print(f"[PY] warning: could not write {out_json}: {e}")

    sys.exit(0 if res["mismatches"] == 0 else 1)

if __name__ == "__main__":
    main()
