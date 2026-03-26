#!/usr/bin/env python3
"""Sync docs code snippets from source files.

Usage:
    python scripts/sync_docs_snippets.py
"""

from __future__ import annotations

import html
import pathlib
import re
import sys
import textwrap

ROOT = pathlib.Path(__file__).resolve().parents[1]

EXAMPLE_FILE = ROOT / "examples" / "basic_endpoint.cpp"
DOCS_INDEX = ROOT / "docs" / "index.html"
DOCS_BLOG = ROOT / "docs" / "blog" / "mcu-udp-vs-tcp.md"

SNIPPET_BEGIN = "// DOCS_SNIPPET_BEGIN:basic_endpoint"
SNIPPET_END = "// DOCS_SNIPPET_END:basic_endpoint"

INDEX_BEGIN = "<!-- BEGIN: BASIC_ENDPOINT_SNIPPET -->"
INDEX_END = "<!-- END: BASIC_ENDPOINT_SNIPPET -->"
BLOG_BEGIN = "<!-- BEGIN: BASIC_ENDPOINT_SNIPPET -->"
BLOG_END = "<!-- END: BASIC_ENDPOINT_SNIPPET -->"


def read_text(path: pathlib.Path) -> str:
    return path.read_text(encoding="utf-8")


def write_text(path: pathlib.Path, content: str) -> None:
    path.write_text(content, encoding="utf-8", newline="\n")


def extract_snippet(source: str, begin_marker: str, end_marker: str) -> str:
    in_block = False
    lines: list[str] = []
    for line in source.splitlines():
        if begin_marker in line:
            in_block = True
            continue
        if end_marker in line:
            in_block = False
            break
        if in_block:
            lines.append(line)

    if not lines:
        raise RuntimeError("Failed to extract snippet block from example source.")

    return textwrap.dedent("\n".join(lines)).strip()


def replace_between_markers(text: str, begin_marker: str, end_marker: str, block: str) -> str:
    pattern = re.compile(re.escape(begin_marker) + r".*?" + re.escape(end_marker), re.S)
    replacement = begin_marker + "\n" + block + "\n" + end_marker
    if not pattern.search(text):
        raise RuntimeError(f"Marker block not found: {begin_marker} ... {end_marker}")
    return pattern.sub(replacement, text, count=1)


def main() -> int:
    source = read_text(EXAMPLE_FILE)
    snippet = extract_snippet(source, SNIPPET_BEGIN, SNIPPET_END)

    html_block = "<pre>" + html.escape(snippet) + "</pre>"
    md_block = "```cpp\n" + snippet + "\n```"

    index_text = read_text(DOCS_INDEX)
    blog_text = read_text(DOCS_BLOG)

    new_index = replace_between_markers(index_text, INDEX_BEGIN, INDEX_END, html_block)
    new_blog = replace_between_markers(blog_text, BLOG_BEGIN, BLOG_END, md_block)

    if new_index != index_text:
        write_text(DOCS_INDEX, new_index)
    if new_blog != blog_text:
        write_text(DOCS_BLOG, new_blog)

    print("Synced snippets: docs/index.html, docs/blog/mcu-udp-vs-tcp.md")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as exc:  # pragma: no cover
        print(f"sync_docs_snippets failed: {exc}", file=sys.stderr)
        raise SystemExit(1)
