#!/usr/bin/env python3
"""Bundle board-run evidence into a single archive for review or handoff."""

from __future__ import annotations

import argparse
import subprocess
import sys
import zipfile
from datetime import datetime, timezone
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
RUNS_ROOT = ROOT / "logs" / "board-runs"
FILE_DESCRIPTIONS = {
    "run-summary.txt": "run metadata and operator notes",
    "board-serial-notes.txt": "captured or summarized board-side serial lines",
    "host-peer.log": "captured host peer output",
    "board-bringup-report-draft.md": "editable maintainer draft used to shape the final report",
    "board-bringup-report.md": "rendered board bring-up report ready for issue filing",
    "board-bringup-issue-preview.md": "dry-run issue preview including the derived title and body",
    "board-run-artifact-manifest.md": "bundle manifest describing the packaged evidence",
}
HELPER_ORDER = [
    "start-host-peer.ps1",
    "build-board.ps1",
    "upload-board.ps1",
    "monitor-board.ps1",
    "render-report.ps1",
    "validate-run.ps1",
    "finalize-report.ps1",
    "file-issue.ps1",
    "bundle-artifacts.ps1",
]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--run-id", help="Run id under logs/board-runs")
    parser.add_argument("--run-dir", type=Path, help="Explicit run directory path")
    parser.add_argument("--output", type=Path, help="Optional zip output path")
    parser.add_argument(
        "--preview-output",
        type=Path,
        help="Optional issue preview output path; defaults to <run-dir>/board-bringup-issue-preview.md",
    )
    parser.add_argument(
        "--manifest-output",
        type=Path,
        help="Optional manifest output path; defaults to <run-dir>/board-run-artifact-manifest.md",
    )
    args = parser.parse_args()
    if not args.run_id and not args.run_dir:
        parser.error("either --run-id or --run-dir is required")
    return args


def resolve_run_dir(args: argparse.Namespace) -> Path:
    if args.run_dir:
        return args.run_dir if args.run_dir.is_absolute() else (ROOT / args.run_dir)
    return RUNS_ROOT / args.run_id


def resolve_output(path: Path | None, default: Path) -> Path:
    if path is None:
        return default
    return path if path.is_absolute() else (ROOT / path)


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


def run(cmd: list[str]) -> subprocess.CompletedProcess[str]:
    completed = subprocess.run(
        cmd,
        cwd=ROOT,
        check=False,
        capture_output=True,
        text=True,
    )
    if completed.returncode != 0:
        if completed.stdout:
            print(completed.stdout, file=sys.stderr, end="")
        if completed.stderr:
            print(completed.stderr, file=sys.stderr, end="")
        raise subprocess.CalledProcessError(completed.returncode, cmd)
    return completed


def helper_files(run_dir: Path) -> list[Path]:
    return [path for name in HELPER_ORDER if (path := run_dir / name).is_file()]


def write_preview(run_dir: Path, preview_path: Path) -> None:
    completed = run([sys.executable, "scripts/file_board_run_issue.py", "--run-dir", str(run_dir), "--dry-run"])
    preview_path.parent.mkdir(parents=True, exist_ok=True)
    preview_path.write_text(completed.stdout, encoding="utf-8", newline="\n")


def collect_files(run_dir: Path, preview_path: Path) -> list[Path]:
    files: list[Path] = []
    for name in [
        "run-summary.txt",
        "board-serial-notes.txt",
        "host-peer.log",
        "board-bringup-report-draft.md",
        "board-bringup-report.md",
    ]:
        path = run_dir / name
        if path.is_file():
            files.append(path)
    if preview_path.is_file() and preview_path not in files:
        files.append(preview_path)
    files.extend(path for path in helper_files(run_dir) if path not in files)
    return files


def render_manifest(run_dir: Path, manifest_path: Path, bundle_path: Path, summary: dict[str, str], files: list[Path]) -> None:
    lines = [
        "# Board Run Artifact Bundle",
        "",
        f"- Run id: `{run_dir.name}`",
        f"- Board path: {summary.get('Board path', '')}",
        f"- Validation result: {summary.get('Run result', '')}",
        f"- Generated at (UTC): {datetime.now(timezone.utc).replace(microsecond=0).isoformat()}",
        f"- Source run directory: `{run_dir}`",
        f"- Bundle file: `{bundle_path.name}`",
        "",
        "## Included Files",
        "",
    ]
    for path in files:
        description = FILE_DESCRIPTIONS.get(path.name, "generated helper or support file")
        lines.append(f"- `{path.name}`: {description}")
    lines.extend([
        "",
        "## Notes",
        "",
        "This bundle is intended for review, handoff, or later evidence lookup.",
        "The issue preview is generated from the current run contents and can be compared directly with the filed issue.",
        "",
    ])
    manifest_path.parent.mkdir(parents=True, exist_ok=True)
    manifest_path.write_text("\n".join(lines), encoding="utf-8", newline="\n")


def create_bundle(bundle_path: Path, files: list[Path]) -> None:
    bundle_path.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(bundle_path, "w", compression=zipfile.ZIP_DEFLATED) as archive:
        for path in files:
            archive.write(path, arcname=path.name)


def main() -> int:
    args = parse_args()
    run_dir = resolve_run_dir(args)
    if not run_dir.is_dir():
        print(f"run directory not found: {run_dir}", file=sys.stderr)
        return 1

    preview_path = resolve_output(args.preview_output, run_dir / "board-bringup-issue-preview.md")
    manifest_path = resolve_output(args.manifest_output, run_dir / "board-run-artifact-manifest.md")
    bundle_path = resolve_output(args.output, run_dir / "board-run-artifacts.zip")

    write_preview(run_dir, preview_path)
    summary = parse_summary(run_dir / "run-summary.txt")
    files = collect_files(run_dir, preview_path)

    render_manifest(run_dir, manifest_path, bundle_path, summary, files + [manifest_path])
    if manifest_path not in files:
        files.append(manifest_path)

    create_bundle(bundle_path, files)
    print(bundle_path)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
