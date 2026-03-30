#!/usr/bin/env python3
"""Render a maintainer-facing C ABI decision snapshot from current repository state."""

from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
REVIEW_INPUTS = [
    "docs/c-abi-stability-gate.md",
    "docs/c-abi-stability-assessment.md",
    "docs/c-abi-downstream-baseline-decision.md",
    "docs/c-abi-stability-decision-playbook.md",
    "docs/c-abi-stable-candidate-change-set.md",
    "docs/c-abi-stable-wording-pack.md",
    "docs/c-abi-surface-v1.json",
    "scripts/evaluate_c_abi_gate.py",
]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--decision", choices=["wait", "stable"], required=True)
    parser.add_argument("--docs-mode", choices=["wait", "stable"])
    parser.add_argument("--downstream-baseline-adopted", action="store_true")
    parser.add_argument("--no-near-term-break", action="store_true")
    parser.add_argument("--run-release-check", action="store_true")
    parser.add_argument("--output", type=Path, help="Output markdown path")
    parser.add_argument("--stdout", action="store_true", help="Print the snapshot to stdout")
    return parser.parse_args()


def run(cmd: list[str]) -> tuple[bool, str]:
    completed = subprocess.run(
        cmd,
        cwd=ROOT,
        check=False,
        capture_output=True,
        text=True,
    )
    output = (completed.stdout or "") + (completed.stderr or "")
    return completed.returncode == 0, output.strip()


def status_line(ok: bool) -> str:
    return "pass" if ok else "fail"


def code_block(text: str) -> str:
    if not text.strip():
        return "```text\n<no output>\n```"
    return "```text\n" + text.rstrip() + "\n```"


def extract_stable_ready(output: str) -> str:
    for line in output.splitlines():
        if line.lower().startswith("stable-ready:"):
            return line.split(":", 1)[1].strip()
    return "unknown"


def decision_label(decision: str) -> str:
    return "keep `wait`" if decision == "wait" else "declare `stable`"


def recommendation(args: argparse.Namespace, docs_mode: str, stable_ready: str) -> str:
    if args.decision == "wait":
        blockers = []
        if args.downstream_baseline_adopted:
            blockers.append("the downstream baseline flag is already set in this snapshot")
        else:
            blockers.append("the downstream baseline is still not explicitly adopted")
        if args.no_near_term_break:
            blockers.append("the no-near-term-break flag is already set in this snapshot")
        else:
            blockers.append("there is still no explicit no-near-term-break position")
        return (
            f"Current candidate: keep `wait`. The checked docs mode is `{docs_mode}`, and the snapshot still shows that {'; '.join(blockers)}. "
            "That means issue #12 should remain blocked and no stable declaration should be merged accidentally."
        )

    if stable_ready == "yes":
        return (
            f"Current candidate: declare `stable`. The gate evaluator reports `stable-ready: yes` for docs mode `{docs_mode}` with the supplied governance flags. "
            "A real stable declaration would still need one coherent docs and release-note change set before issue #12 should move."
        )

    return (
        f"Current candidate: declare `stable`, but this snapshot is not ready. The gate evaluator reports `stable-ready: {stable_ready}` for docs mode `{docs_mode}`. "
        "Keep the ABI in `wait` until the missing governance promises and stable-mode docs change set are made intentionally."
    )


def main() -> int:
    args = parse_args()
    docs_mode = args.docs_mode or args.decision

    docs_ok, docs_output = run([sys.executable, "scripts/check_c_abi_docs.py", "--mode", docs_mode])
    surface_ok, surface_output = run([sys.executable, "scripts/check_c_abi_surface.py"])

    gate_cmd = [sys.executable, "scripts/evaluate_c_abi_gate.py", "--docs-mode", docs_mode]
    gate_cmd_display = ["python", "scripts/evaluate_c_abi_gate.py", "--docs-mode", docs_mode]
    if args.downstream_baseline_adopted:
        gate_cmd.append("--downstream-baseline-adopted")
        gate_cmd_display.append("--downstream-baseline-adopted")
    if args.no_near_term_break:
        gate_cmd.append("--no-near-term-break")
        gate_cmd_display.append("--no-near-term-break")
    if args.run_release_check:
        gate_cmd.append("--run-release-check")
        gate_cmd_display.append("--run-release-check")
    gate_ok, gate_output = run(gate_cmd)

    release_status = "not run"
    release_output = "release_check not run in this snapshot"
    if args.run_release_check:
        release_ok, release_output = run([sys.executable, "scripts/release_check.py"])
        release_status = status_line(release_ok)

    stable_ready = extract_stable_ready(gate_output)
    lines = [
        "# C ABI Decision Snapshot",
        "",
        "Suggested issue title: `abi: decision for current C ABI baseline`",
        "Suggested labels: `api`, `decision`",
        "",
        "## Candidate Decision",
        "",
        f"- Requested decision: {decision_label(args.decision)}",
        f"- Docs mode checked: `{docs_mode}`",
        f"- Downstream baseline adopted flag: `{'yes' if args.downstream_baseline_adopted else 'no'}`",
        f"- No-near-term-break flag: `{'yes' if args.no_near_term_break else 'no'}`",
        f"- Release check run in this snapshot: `{'yes' if args.run_release_check else 'no'}`",
        f"- Gate evaluator summary: stable-ready: {stable_ready}",
        "",
        "## Decision Inputs To Review",
        "",
    ]
    lines.extend(f"- `{item}`" for item in REVIEW_INPUTS)
    lines.extend([
        "",
        "## Validation Status",
        "",
        f"- `python scripts/check_c_abi_docs.py --mode {docs_mode}`: `{status_line(docs_ok)}`",
        f"- `python scripts/check_c_abi_surface.py`: `{status_line(surface_ok)}`",
        f"- `{' '.join(gate_cmd_display)}`: `{status_line(gate_ok)}`",
        f"- `python scripts/release_check.py`: `{release_status}`",
        "",
        "## Repository Read",
        "",
        recommendation(args, docs_mode, stable_ready),
        "",
        "## check_c_abi_docs Output",
        "",
        code_block(docs_output),
        "",
        "## check_c_abi_surface Output",
        "",
        code_block(surface_output),
        "",
        "## evaluate_c_abi_gate Output",
        "",
        code_block(gate_output),
        "",
        "## release_check Output",
        "",
        code_block(release_output),
        "",
        "## Notes",
        "",
        "Use this snapshot as a maintainer summary or paste the relevant sections into `.github/ISSUE_TEMPLATE/c-abi-stability-decision.md`.",
        "It is a status render, not a substitute for an intentional policy decision.",
        "",
    ])
    snapshot = "\n".join(lines)

    if args.stdout or args.output is None:
        print(snapshot)

    if args.output is not None:
        output_path = args.output if args.output.is_absolute() else (ROOT / args.output)
        output_path.parent.mkdir(parents=True, exist_ok=True)
        output_path.write_text(snapshot, encoding="utf-8", newline="\n")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
