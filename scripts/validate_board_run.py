#!/usr/bin/env python3
"""Validate a board-run workspace before or after a hardware run.

Modes:
- preflight: check that the run pack and templates are present and coherent
- report: check that the rendered report exists and no obvious placeholders remain
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
RUNS_ROOT = ROOT / "logs" / "board-runs"

CORE_FILES = [
    "run-summary.txt",
    "board-serial-notes.txt",
    "board-bringup-report-draft.md",
]
WINDOWS_HELPERS = [
    "start-host-peer.ps1",
    "build-board.ps1",
    "upload-board.ps1",
    "monitor-board.ps1",
    "render-report.ps1",
]
REPORT_PLACEHOLDERS = [
    "pass|partial|fail",
    "<paste here>",
    "<missing",
    "<no host-peer.log captured>",
    "<no board-serial-notes.txt content captured>",
    "Describe anything that did not match the documented path.",
    "Anything else a maintainer should know to reproduce or interpret the run.",
]
SUMMARY_REQUIRED_KEYS = [
    "Board path",
    "Run result",
    "Date",
    "Operator",
    "Board",
    "Ethernet shield / chipset",
    "PlatformIO environment",
    "Host OS",
    "Host peer command",
    "Board IP / port",
    "Host IP / port",
]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--run-id", help="Run id under logs/board-runs")
    parser.add_argument("--run-dir", type=Path, help="Explicit run directory path")
    parser.add_argument("--mode", choices=["preflight", "report"], default="preflight")
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
    values: dict[str, str] = {}
    for raw_line in read_text(path).splitlines():
        if ":" not in raw_line:
            continue
        key, value = raw_line.split(":", 1)
        values[key.strip()] = value.strip()
    return values


def check_preflight(run_dir: Path) -> list[str]:
    errors: list[str] = []
    for rel in CORE_FILES:
        if not (run_dir / rel).is_file():
            errors.append(f"missing required file: {rel}")

    summary_path = run_dir / "run-summary.txt"
    if summary_path.is_file():
        summary = parse_summary(summary_path)
        for key in SUMMARY_REQUIRED_KEYS:
            if key not in summary:
                errors.append(f"run-summary missing key: {key}")

    existing_helpers = [name for name in WINDOWS_HELPERS if (run_dir / name).is_file()]
    if existing_helpers and len(existing_helpers) != len(WINDOWS_HELPERS):
        missing = [name for name in WINDOWS_HELPERS if name not in existing_helpers]
        errors.append("windows helper set is incomplete: " + ", ".join(missing))

    host_log = run_dir / "host-peer.log"
    if host_log.exists() and not host_log.is_file():
        errors.append("host-peer.log exists but is not a file")

    return errors


def check_report(run_dir: Path) -> list[str]:
    errors = check_preflight(run_dir)
    report_path = run_dir / "board-bringup-report.md"
    if not report_path.is_file():
        errors.append("missing required file: board-bringup-report.md")
        return errors

    text = read_text(report_path)
    for marker in REPORT_PLACEHOLDERS:
        if marker in text:
            errors.append(f"report still contains placeholder text: {marker}")

    if "## Observed Host-Side Lines" in text and "host-peer.log" not in "":
        pass

    summary = parse_summary(run_dir / "run-summary.txt") if (run_dir / "run-summary.txt").is_file() else {}
    run_result = summary.get("Run result", "")
    if run_result in {"", "pass|partial|fail"}:
        errors.append("run-summary Run result is still unset")

    host_log = run_dir / "host-peer.log"
    if not host_log.is_file():
        errors.append("host-peer.log is missing; report mode expects captured host output")

    return errors


def main() -> int:
    args = parse_args()
    run_dir = resolve_run_dir(args)
    if not run_dir.is_dir():
        print(f"run directory not found: {run_dir}", file=sys.stderr)
        return 1

    errors = check_preflight(run_dir) if args.mode == "preflight" else check_report(run_dir)
    if errors:
        for item in errors:
            print(item, file=sys.stderr)
        return 1

    print(f"validate_board_run passed ({args.mode})")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
