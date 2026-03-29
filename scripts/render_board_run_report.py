#!/usr/bin/env python3
"""Render a board bring-up report from a run directory."""

from __future__ import annotations

import argparse
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
RUNS_ROOT = ROOT / "logs" / "board-runs"

ENV_KEYS = [
    "Date",
    "Operator",
    "Board",
    "Ethernet shield / chipset",
    "PlatformIO environment",
    "Host OS",
    "Host peer command",
    "Board IP / port",
    "Host IP / port",
    "Local edits from defaults",
]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--run-id", help="Run id under logs/board-runs")
    parser.add_argument("--run-dir", type=Path, help="Explicit run directory path")
    parser.add_argument("--output", type=Path, help="Output markdown path")
    parser.add_argument("--stdout", action="store_true", help="Print report to stdout")
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


def parse_sections(path: Path) -> dict[str, str]:
    if not path.is_file():
        return {}
    text = read_text(path)
    pattern = re.compile(r"^##\s+(.+?)\n", re.MULTILINE)
    matches = list(pattern.finditer(text))
    sections: dict[str, str] = {}
    for index, match in enumerate(matches):
        start = match.end()
        end = matches[index + 1].start() if index + 1 < len(matches) else len(text)
        sections[match.group(1).strip()] = text[start:end].strip()
    return sections


def load_optional(path: Path, fallback: str) -> str:
    if not path.is_file():
        return fallback
    content = read_text(path).strip()
    return content or fallback


def code_block(text: str) -> str:
    return f"```text\n{text.rstrip()}\n```"


def main() -> int:
    args = parse_args()
    run_dir = resolve_run_dir(args)
    summary_path = run_dir / "run-summary.txt"
    notes_path = run_dir / "board-serial-notes.txt"
    host_log_path = run_dir / "host-peer.log"
    draft_path = run_dir / "board-bringup-report-draft.md"

    summary = parse_summary(summary_path)
    draft_sections = parse_sections(draft_path)

    report_summary = summary.get("Board path", "<missing board path>")
    validation_result = summary.get("Run result", "<missing run result>")
    checklist = draft_sections.get(
        "Bring-Up Checklist",
        "- [ ] board flashed successfully\n"
        "- [ ] serial monitor opened successfully\n"
        "- [ ] host peer started on the expected address and port\n"
        "- [ ] at least one side showed handshake progress\n"
        "- [ ] both sides reached connected state `2`\n"
        "- [ ] board-to-host app payload was observed\n"
        "- [ ] host-to-board app payload was observed",
    )
    failures = draft_sections.get(
        "Failures Or Deviations",
        "Describe anything that did not match the documented path.",
    )
    notes_section = draft_sections.get(
        "Notes",
        summary.get("Notes", "Anything else a maintainer should know to reproduce or interpret the run."),
    )

    env_lines = [f"- {key}: {summary.get(key, '')}" for key in ENV_KEYS]

    board_lines = load_optional(
        notes_path,
        "<no board-serial-notes.txt content captured>",
    )
    host_lines = load_optional(
        host_log_path,
        "<no host-peer.log captured>",
    )

    report = "\n".join([
        "## Summary",
        "",
        report_summary,
        "",
        "## Validation Result",
        "",
        validation_result,
        "",
        "## Environment",
        "",
        *env_lines,
        "",
        "## Bring-Up Checklist",
        "",
        checklist,
        "",
        "## Observed Board-Side Lines",
        "",
        code_block(board_lines),
        "",
        "## Observed Host-Side Lines",
        "",
        code_block(host_lines),
        "",
        "## Failures Or Deviations",
        "",
        failures,
        "",
        "## Notes",
        "",
        notes_section,
        "",
    ])

    output_path = args.output
    if output_path is None and not args.stdout:
        output_path = run_dir / "board-bringup-report.md"
    if output_path is not None:
        if not output_path.is_absolute():
            output_path = ROOT / output_path
        output_path.parent.mkdir(parents=True, exist_ok=True)
        output_path.write_text(report, encoding="utf-8", newline="\n")
        print(output_path)
    if args.stdout:
        print(report)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
