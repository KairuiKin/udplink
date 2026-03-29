#!/usr/bin/env python3
"""Run a command, stream output to console, and tee it to a log file.

Example:
    python scripts/run_peer_capture.py --log logs/peer.txt -- ./build/rudp_example_udp_peer 192.168.1.100 8889 192.168.1.177 8888
"""

from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--log", required=True, help="Path to the output log file")
    parser.add_argument("command", nargs=argparse.REMAINDER)
    args = parser.parse_args()
    if args.command and args.command[0] == "--":
        args.command = args.command[1:]
    if not args.command:
        parser.error("a command is required after --")
    return args


def main() -> int:
    args = parse_args()
    log_path = Path(args.log)
    if not log_path.is_absolute():
        log_path = ROOT / log_path
    log_path.parent.mkdir(parents=True, exist_ok=True)

    print("+", " ".join(args.command))
    with log_path.open("w", encoding="utf-8", newline="\n") as log_file:
        proc = subprocess.Popen(
            args.command,
            cwd=ROOT,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            encoding="utf-8",
            errors="replace",
        )
        assert proc.stdout is not None
        for line in proc.stdout:
            sys.stdout.write(line)
            log_file.write(line)
        return_code = proc.wait()

    if return_code != 0:
        print(f"command failed with exit code {return_code}; log saved to {log_path}", file=sys.stderr)
        return return_code

    print(f"log saved to {log_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
