#!/usr/bin/env python3
"""Render and validate a board-run report in one step."""

from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
RUNS_ROOT = ROOT / "logs" / "board-runs"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--run-id", help="Run id under logs/board-runs")
    parser.add_argument("--run-dir", type=Path, help="Explicit run directory path")
    args = parser.parse_args()
    if not args.run_id and not args.run_dir:
        parser.error("either --run-id or --run-dir is required")
    return args


def resolve_run_dir(args: argparse.Namespace) -> Path:
    if args.run_dir:
        return args.run_dir if args.run_dir.is_absolute() else (ROOT / args.run_dir)
    return RUNS_ROOT / args.run_id


def run(cmd: list[str]) -> None:
    subprocess.run(cmd, cwd=ROOT, check=True)


def main() -> int:
    args = parse_args()
    run_dir = resolve_run_dir(args)
    if not run_dir.is_dir():
        print(f"run directory not found: {run_dir}", file=sys.stderr)
        return 1

    run([sys.executable, "scripts/render_board_run_report.py", "--run-dir", str(run_dir)])
    run([sys.executable, "scripts/validate_board_run.py", "--run-dir", str(run_dir), "--mode", "report"])
    print(run_dir / "board-bringup-report.md")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
