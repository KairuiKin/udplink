#!/usr/bin/env python3
"""Check that key C ABI docs agree on the repository's current decision mode.

Modes:
- wait: current repository stance; ABI implemented but not yet declared stable
- stable: future stance once maintainers intentionally bless the downstream
  baseline and declare the current v1 ABI stable

The checker validates both:
- required snippets for the selected mode
- forbidden snippets that would indicate a mixed wait/stable doc state
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

REQUIRED_FILES = [
    "docs/c-abi-stability-gate.md",
    "docs/c-abi-stability-assessment.md",
    "docs/c-abi-downstream-baseline-decision.md",
    "docs/c-abi-stability-decision-playbook.md",
    "docs/c-abi-compatibility.md",
    "docs/c-api-quickstart.md",
    "docs/project-status.md",
    "docs/release-checklist.md",
    "docs/_sidebar.md",
    "docs/index.html",
    "examples/c_api/README.md",
]

COMMON_REQUIRED = {
    "docs/c-abi-stability-gate.md": [
        "docs/c-abi-downstream-baseline-decision.md",
    ],
    "docs/c-abi-stability-assessment.md": [
        "docs/c-abi-downstream-baseline-decision.md",
    ],
    "docs/c-abi-downstream-baseline-decision.md": [
        "examples/c_api/install_consume/",
    ],
    "docs/c-abi-stability-decision-playbook.md": [
        "Path A: Keep Wait",
        "Path B: Declare Stable",
    ],
    "docs/project-status.md": [
        "docs/c-abi-stability-decision-playbook.md",
        "docs/c-abi-stable-candidate-change-set.md",
    ],
    "docs/release-checklist.md": [
        "docs/c-abi-stability-decision-playbook.md",
        "docs/c-abi-downstream-baseline-decision.md",
    ],
    "docs/_sidebar.md": [
        "[C ABI Decision Playbook](c-abi-stability-decision-playbook.md)",
        "[C ABI Stable Candidate](c-abi-stable-candidate-change-set.md)",
    ],
    "docs/index.html": [
        'href="c-abi-stability-decision-playbook.md"',
        'href="c-abi-downstream-baseline-decision.md"',
        'href="c-abi-stable-candidate-change-set.md"',
    ],
    "examples/c_api/README.md": [
        "docs/c-abi-downstream-baseline-decision.md",
    ],
}

MODE_REQUIRED = {
    "wait": {
        "docs/c-abi-stability-gate.md": [
            "the C ABI is not yet declared long-term stable",
        ],
        "docs/c-abi-stability-assessment.md": [
            "Current answer: not yet.",
            "Recommendation: wait before declaring the current C ABI stable.",
        ],
        "docs/c-abi-downstream-baseline-decision.md": [
            "Decision status: proposed, not yet adopted.",
            "Current recommendation: do not bless it yet.",
        ],
        "docs/c-abi-stability-decision-playbook.md": [
            "Current recommended answer: keep `wait`.",
            "python scripts/check_c_abi_docs.py --mode wait",
        ],
        "docs/c-abi-compatibility.md": [
            "it is not yet declared long-term stable",
        ],
        "docs/c-api-quickstart.md": [
            "the ABI is still intentionally small and not yet declared stable",
        ],
        "docs/project-status.md": [
            "the current C ABI is implemented, but it is not yet declared stable",
            "maintainers have not yet blessed `examples/c_api/install_consume/` as the first supported downstream baseline",
        ],
        "docs/release-checklist.md": [
            "python scripts/check_c_abi_docs.py --mode wait",
        ],
    },
    "stable": {
        "docs/c-abi-stability-gate.md": [
            "A release may say the C ABI is stable only if maintainers do all of the following together:",
        ],
        "docs/c-abi-stability-assessment.md": [
            "Current answer: ready.",
            "Recommendation: the current C ABI baseline may be declared stable.",
        ],
        "docs/c-abi-downstream-baseline-decision.md": [
            "Decision status: adopted.",
            "Current recommendation: bless it.",
        ],
        "docs/c-abi-stability-decision-playbook.md": [
            "Current recommended answer: declare `stable`.",
            "python scripts/check_c_abi_docs.py --mode stable",
        ],
        "docs/c-abi-compatibility.md": [
            "the current v1 ABI is now treated as an explicitly stable baseline",
        ],
        "docs/c-api-quickstart.md": [
            "the current C ABI baseline is now treated as stable",
        ],
        "docs/project-status.md": [
            "the current C ABI baseline is declared stable",
            "`examples/c_api/install_consume/` is the first supported downstream baseline",
        ],
        "docs/release-checklist.md": [
            "python scripts/check_c_abi_docs.py --mode stable",
        ],
    },
}

MODE_FORBIDDEN = {
    "wait": {
        "docs/c-abi-stability-assessment.md": [
            "Current answer: ready.",
            "Recommendation: the current C ABI baseline may be declared stable.",
        ],
        "docs/c-abi-downstream-baseline-decision.md": [
            "Decision status: adopted.",
            "Current recommendation: bless it.",
        ],
        "docs/c-abi-stability-decision-playbook.md": [
            "Current recommended answer: declare `stable`.",
        ],
        "docs/c-abi-compatibility.md": [
            "the current v1 ABI is now treated as an explicitly stable baseline",
        ],
        "docs/c-api-quickstart.md": [
            "the current C ABI baseline is now treated as stable",
        ],
        "docs/project-status.md": [
            "the current C ABI baseline is declared stable",
            "`examples/c_api/install_consume/` is the first supported downstream baseline",
        ],
        "docs/release-checklist.md": [
            "python scripts/check_c_abi_docs.py --mode stable",
        ],
    },
    "stable": {
        "docs/c-abi-stability-gate.md": [
            "the C ABI is not yet declared long-term stable",
        ],
        "docs/c-abi-stability-assessment.md": [
            "Current answer: not yet.",
            "Recommendation: wait before declaring the current C ABI stable.",
        ],
        "docs/c-abi-downstream-baseline-decision.md": [
            "Decision status: proposed, not yet adopted.",
            "Current recommendation: do not bless it yet.",
        ],
        "docs/c-abi-stability-decision-playbook.md": [
            "Current recommended answer: keep `wait`.",
        ],
        "docs/c-abi-compatibility.md": [
            "it is not yet declared long-term stable",
        ],
        "docs/c-api-quickstart.md": [
            "the ABI is still intentionally small and not yet declared stable",
        ],
        "docs/project-status.md": [
            "the current C ABI is implemented, but it is not yet declared stable",
            "maintainers have not yet blessed `examples/c_api/install_consume/` as the first supported downstream baseline",
        ],
        "docs/release-checklist.md": [
            "python scripts/check_c_abi_docs.py --mode wait",
        ],
    },
}


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--mode", choices=sorted(MODE_REQUIRED), default="wait")
    return parser.parse_args()


def read_text(rel: str) -> str:
    return (ROOT / rel).read_text(encoding="utf-8")


def collect_expected(mode: str) -> dict[str, list[str]]:
    expected = {rel: list(snippets) for rel, snippets in COMMON_REQUIRED.items()}
    for rel, snippets in MODE_REQUIRED[mode].items():
        expected.setdefault(rel, []).extend(snippets)
    return expected


def main() -> int:
    args = parse_args()
    missing_files: list[str] = []
    missing_snippets: list[str] = []
    forbidden_snippets: list[str] = []

    for rel in REQUIRED_FILES:
        if not (ROOT / rel).is_file():
            missing_files.append(rel)

    if missing_files:
        for rel in missing_files:
            print(f"missing file: {rel}", file=sys.stderr)
        return 1

    expected = collect_expected(args.mode)

    for rel, snippets in expected.items():
        text = read_text(rel)
        for snippet in snippets:
            if snippet not in text:
                missing_snippets.append(f"{rel}: missing snippet for mode={args.mode}: {snippet}")

    for rel, snippets in MODE_FORBIDDEN[args.mode].items():
        text = read_text(rel)
        for snippet in snippets:
            if snippet in text:
                forbidden_snippets.append(f"{rel}: forbidden snippet for mode={args.mode}: {snippet}")

    if missing_snippets or forbidden_snippets:
        for item in missing_snippets:
            print(item, file=sys.stderr)
        for item in forbidden_snippets:
            print(item, file=sys.stderr)
        return 1

    print(f"check_c_abi_docs passed (mode={args.mode})")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
