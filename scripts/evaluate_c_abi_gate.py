#!/usr/bin/env python3
"""Evaluate the repository against the documented C ABI stability gate."""

from __future__ import annotations

import argparse
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


@dataclass
class GateResult:
    title: str
    met: bool
    detail: str


REQUIRED_FILES = [
    "include/rudp/rudp_c.h",
    "src/rudp_c.cpp",
    "tests/c_api_test.cpp",
    "tests/install_consume_c/CMakeLists.txt",
    "tests/install_consume_c/main.c",
    "examples/c_api/install_consume/CMakeLists.txt",
    "examples/c_api/install_consume/main.c",
    "docs/c-api-quickstart.md",
    "docs/c-abi-draft.md",
    "docs/c-abi-compatibility.md",
    "docs/c-abi-stability-gate.md",
    "docs/c-abi-stability-assessment.md",
    "docs/c-abi-downstream-baseline-decision.md",
    "docs/c-abi-stability-decision-playbook.md",
    "docs/c-abi-surface-v1.json",
    "MAINTAIN.md",
    "docs/release-checklist.md",
]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--docs-mode", choices=["wait", "stable"], default="wait")
    parser.add_argument("--downstream-baseline-adopted", action="store_true")
    parser.add_argument("--no-near-term-break", action="store_true")
    parser.add_argument("--run-release-check", action="store_true")
    parser.add_argument("--require-stable-ready", action="store_true")
    return parser.parse_args()


def run(cmd: list[str]) -> tuple[bool, str]:
    proc = subprocess.run(
        cmd,
        cwd=ROOT,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        encoding="utf-8",
        errors="replace",
    )
    output = proc.stdout.strip()
    return proc.returncode == 0, output


def check_required_files() -> list[str]:
    return [rel for rel in REQUIRED_FILES if not (ROOT / rel).is_file()]


def evaluate_surface_discipline() -> GateResult:
    header = (ROOT / "include/rudp/rudp_c.h").read_text(encoding="utf-8")
    forbidden = ["rudp_manager", "manager_handle", "manager_v1"]
    found = [token for token in forbidden if token in header]
    if found:
        return GateResult(
            "1. Surface discipline is still intact",
            False,
            "manager-layer C ABI markers found in include/rudp/rudp_c.h: " + ", ".join(found),
        )
    return GateResult(
        "1. Surface discipline is still intact",
        True,
        "public C header remains endpoint-centric and does not expose manager-layer C ABI markers",
    )


def evaluate_validation(args: argparse.Namespace) -> GateResult:
    checks: list[tuple[str, bool, str]] = []
    checks.append((
        "check_c_abi_docs",
        *run(["python", "scripts/check_c_abi_docs.py", "--mode", args.docs_mode]),
    ))
    checks.append((
        "check_c_abi_surface",
        *run(["python", "scripts/check_c_abi_surface.py"]),
    ))
    if args.run_release_check:
        checks.append((
            "release_check",
            *run(["python", "scripts/release_check.py"]),
        ))

    failures = [(name, output) for name, ok, output in checks if not ok]
    if failures:
        failure_text = "; ".join(
            f"{name} failed: {(output.splitlines() or ['no output'])[-1]}" for name, output in failures
        )
        return GateResult(
            "2. ABI-sensitive validation passes consistently",
            False,
            failure_text,
        )

    detail = "docs state and surface snapshot checks passed"
    if args.run_release_check:
        detail += "; release_check also passed"
    else:
        detail += "; release_check not run in this invocation"
    return GateResult("2. ABI-sensitive validation passes consistently", True, detail)


def evaluate_install_shape() -> GateResult:
    cmake_text = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
    required_snippets = [
        'install(DIRECTORY include/rudp',
        'PATTERN "*.hpp" PATTERN "*.h"',
        'install(EXPORT rudpTargets',
    ]
    missing = [snippet for snippet in required_snippets if snippet not in cmake_text]
    if missing:
        return GateResult(
            "3. Install and packaging shape are no longer in flux",
            False,
            "missing install/export snippets in CMakeLists.txt",
        )
    return GateResult(
        "3. Install and packaging shape are no longer in flux",
        True,
        "CMake install/export rules still install public headers and exported package metadata",
    )


def evaluate_docs_alignment() -> GateResult:
    missing_files = check_required_files()
    if missing_files:
        return GateResult(
            "4. Documentation agrees on the contract",
            False,
            "missing required files: " + ", ".join(missing_files),
        )
    return GateResult(
        "4. Documentation agrees on the contract",
        True,
        "all required C ABI docs and supporting files are present",
    )


def evaluate_downstream(args: argparse.Namespace) -> GateResult:
    if args.downstream_baseline_adopted:
        return GateResult(
            "5. There is at least one credible downstream use case",
            True,
            "operator explicitly marked the in-repo maintained C consumer as the adopted downstream baseline",
        )
    return GateResult(
        "5. There is at least one credible downstream use case",
        False,
        "maintained in-repo example exists, but this invocation did not declare the downstream baseline adopted",
    )


def evaluate_no_break(args: argparse.Namespace) -> GateResult:
    if args.no_near_term_break:
        return GateResult(
            "6. No known near-term breaking change is queued",
            True,
            "operator explicitly marked the current v1 ABI as not expected to need a near-term break",
        )
    return GateResult(
        "6. No known near-term breaking change is queued",
        False,
        "no explicit no-near-term-break confirmation was provided to this invocation",
    )


def print_report(results: list[GateResult], args: argparse.Namespace) -> None:
    print(f"docs mode: {args.docs_mode}")
    print(
        "governance flags: downstream_baseline_adopted=%s no_near_term_break=%s"
        % (str(args.downstream_baseline_adopted).lower(), str(args.no_near_term_break).lower())
    )
    print()
    for item in results:
        status = "met" if item.met else "open"
        print(f"[{status}] {item.title}")
        print(f"  {item.detail}")
    print()


def main() -> int:
    args = parse_args()
    results = [
        evaluate_surface_discipline(),
        evaluate_validation(args),
        evaluate_install_shape(),
        evaluate_docs_alignment(),
        evaluate_downstream(args),
        evaluate_no_break(args),
    ]
    print_report(results, args)

    stable_ready = all(item.met for item in results)
    print("stable-ready:", "yes" if stable_ready else "no")

    if args.require_stable_ready and not stable_ready:
        return 1

    validation_failed = any(not item.met for item in results[:4])
    return 1 if validation_failed else 0


if __name__ == "__main__":
    raise SystemExit(main())
