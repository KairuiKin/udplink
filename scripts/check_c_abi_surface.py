#!/usr/bin/env python3
"""Validate the public C ABI surface against the checked-in v1 baseline."""

from __future__ import annotations

import argparse
import difflib
import json
import re
import sys
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_HEADER = ROOT / "include" / "rudp" / "rudp_c.h"
DEFAULT_BASELINE = ROOT / "docs" / "c-abi-surface-v1.json"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--header", type=Path, default=DEFAULT_HEADER)
    parser.add_argument("--baseline", type=Path, default=DEFAULT_BASELINE)
    parser.add_argument(
        "--write-baseline",
        action="store_true",
        help="write the current parsed surface to the baseline path",
    )
    return parser.parse_args()


def read_text(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def strip_line_comments(text: str) -> str:
    return re.sub(r"//.*$", "", text, flags=re.MULTILINE)


def normalize_item(text: str) -> str:
    text = re.sub(r"\s+", " ", text.strip())
    text = re.sub(r"\s*\*\s*", "*", text)
    text = re.sub(r"(?<!\()(\*+)(?=[A-Za-z_])", lambda match: match.group(1) + " ", text)
    text = re.sub(r"\s*,\s*", ", ", text)
    text = re.sub(r"\(\s*", "(", text)
    text = re.sub(r"\s*\)", ")", text)
    text = re.sub(r"\s*;\s*$", "", text)
    return text.strip()


def parse_fields(block: str) -> list[str]:
    fields: list[str] = []
    for item in block.split(";"):
        normalized = normalize_item(item)
        if normalized:
            fields.append(normalized)
    return fields


def parse_enum_values(block: str) -> list[str]:
    values: list[str] = []
    for item in block.split(","):
        normalized = normalize_item(item)
        if normalized:
            values.append(normalized)
    return values


def parse_callbacks(text: str) -> list[str]:
    pattern = re.compile(
        r"typedef\s+([^;]*\(\*\s*(rudp_[A-Za-z0-9_]+)\s*\)\s*\([^;]*\))\s*;",
        re.MULTILINE,
    )
    callbacks = [normalize_item(match.group(1)) for match in pattern.finditer(text)]
    return sorted(callbacks)


def parse_opaque_handles(text: str) -> list[dict[str, str]]:
    pattern = re.compile(r"typedef\s+struct\s+(\w+)\s+(\w+)\s*;")
    handles: list[dict[str, str]] = []
    for match in pattern.finditer(text):
        tag, alias = match.groups()
        if tag == alias and alias.endswith("_handle"):
            handles.append({"tag": tag, "alias": alias})
    return sorted(handles, key=lambda item: item["alias"])


def parse_enums(text: str) -> list[dict[str, Any]]:
    pattern = re.compile(r"typedef\s+enum\s+(\w+)\s*\{(.*?)\}\s*(\w+)\s*;", re.DOTALL)
    enums: list[dict[str, Any]] = []
    for match in pattern.finditer(text):
        tag, body, alias = match.groups()
        enums.append({
            "tag": tag,
            "alias": alias,
            "values": parse_enum_values(body),
        })
    return sorted(enums, key=lambda item: item["alias"])


def parse_structs(text: str) -> list[dict[str, Any]]:
    pattern = re.compile(r"typedef\s+struct\s+(\w+)\s*\{(.*?)\}\s*(\w+)\s*;", re.DOTALL)
    structs: list[dict[str, Any]] = []
    for match in pattern.finditer(text):
        tag, body, alias = match.groups()
        structs.append({
            "tag": tag,
            "alias": alias,
            "fields": parse_fields(body),
        })
    return sorted(structs, key=lambda item: item["alias"])


def parse_functions(text: str) -> list[str]:
    pattern = re.compile(
        r"(^\s*(?!typedef\b)[A-Za-z_][^;]*?\brudp_[A-Za-z0-9_]+\s*\([^;]*\)\s*;)",
        re.MULTILINE,
    )
    functions = [normalize_item(match.group(1)) for match in pattern.finditer(text)]
    return sorted(functions)


def parse_abi_version(text: str) -> int:
    match = re.search(r"#define\s+RUDP_ABI_VERSION\s+(\d+)u", text)
    if not match:
        raise ValueError("RUDP_ABI_VERSION not found in header")
    return int(match.group(1))


def collect_surface(header_path: Path) -> dict[str, Any]:
    raw_text = read_text(header_path)
    text = strip_line_comments(raw_text)
    return {
        "abi_version": parse_abi_version(raw_text),
        "opaque_handles": parse_opaque_handles(text),
        "callbacks": parse_callbacks(text),
        "enums": parse_enums(text),
        "structs": parse_structs(text),
        "functions": parse_functions(text),
    }


def dump_json(payload: dict[str, Any]) -> str:
    return json.dumps(payload, indent=2, sort_keys=True) + "\n"


def main() -> int:
    args = parse_args()
    current = collect_surface(args.header)

    if args.write_baseline:
        args.baseline.write_text(dump_json(current), encoding="utf-8")
        print(f"wrote baseline: {args.baseline}")
        return 0

    expected = json.loads(read_text(args.baseline))
    current_text = dump_json(current)
    expected_text = dump_json(expected)

    if current_text != expected_text:
        diff = difflib.unified_diff(
            expected_text.splitlines(),
            current_text.splitlines(),
            fromfile=str(args.baseline),
            tofile=str(args.header),
            lineterm="",
        )
        for line in diff:
            print(line, file=sys.stderr)
        return 1

    print(f"check_c_abi_surface passed ({args.baseline.name})")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())


