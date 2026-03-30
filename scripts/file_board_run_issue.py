#!/usr/bin/env python3
"""Preview or file a board bring-up issue from a run directory."""

from __future__ import annotations

import argparse
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
RUNS_ROOT = ROOT / "logs" / "board-runs"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--run-id", help="Run id under logs/board-runs")
    parser.add_argument("--run-dir", type=Path, help="Explicit run directory path")
    parser.add_argument("--repo", help="Optional owner/repo override for gh issue create")
    parser.add_argument("--title", help="Optional explicit issue title")
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Print the derived issue title and body instead of creating the issue",
    )
    args = parser.parse_args()
    if not args.run_id and not args.run_dir:
        parser.error("either --run-id or --run-dir is required")
    return args


def resolve_run_dir(args: argparse.Namespace) -> Path:
    if args.run_dir:
        return args.run_dir if args.run_dir.is_absolute() else (ROOT / args.run_dir)
    return RUNS_ROOT / args.run_id


def read_text(path: Path) -> str:
    return path.read_text(encoding="utf-8-sig").replace("\r\n", "\n")


def parse_summary(path: Path) -> dict[str, str]:
    summary: dict[str, str] = {}
    for raw_line in read_text(path).splitlines():
        line = raw_line.strip()
        if not line or ":" not in line:
            continue
        key, value = line.split(":", 1)
        summary[key.strip()] = value.strip()
    return summary


def run(cmd: list[str], *, capture: bool = False) -> subprocess.CompletedProcess[str]:
    completed = subprocess.run(
        cmd,
        cwd=ROOT,
        check=False,
        capture_output=capture,
        text=True,
    )
    if completed.returncode != 0:
        if capture:
            if completed.stdout:
                print(completed.stdout, file=sys.stderr, end="")
            if completed.stderr:
                print(completed.stderr, file=sys.stderr, end="")
        raise subprocess.CalledProcessError(completed.returncode, cmd)
    return completed


def derive_title(summary: dict[str, str]) -> str:
    board_path = summary.get("Board path", "<board path>").strip()
    return f"board: bring-up report for {board_path}"


def main() -> int:
    args = parse_args()
    run_dir = resolve_run_dir(args)
    if not run_dir.is_dir():
        print(f"run directory not found: {run_dir}", file=sys.stderr)
        return 1

    run([sys.executable, "scripts/finalize_board_run.py", "--run-dir", str(run_dir)], capture=True)

    report_path = run_dir / "board-bringup-report.md"
    if not report_path.is_file():
        print(f"missing board-bringup-report.md in {run_dir}", file=sys.stderr)
        return 1

    summary = parse_summary(run_dir / "run-summary.txt")
    title = args.title or derive_title(summary)
    body = read_text(report_path).rstrip() + "\n"

    if args.dry_run:
        print(f"Title: {title}")
        print("")
        print(body, end="")
        return 0

    if shutil.which("gh") is None:
        print("gh CLI not found in PATH", file=sys.stderr)
        return 1

    cmd = [
        "gh",
        "issue",
        "create",
        "--title",
        title,
        "--body-file",
        str(report_path),
        "--label",
        "validation",
        "--label",
        "embedded",
    ]
    if args.repo:
        cmd.extend(["--repo", args.repo])
    run(cmd)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
