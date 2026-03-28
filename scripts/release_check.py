#!/usr/bin/env python3
"""Run the local release verification flow for udplink.

This script performs the same core checks that the project now expects before
cutting a release:

1. Sync docs snippets.
2. Configure/build the main project in Release mode.
3. Run self/reliability/manager/bench executables.
4. Build and install the package.
5. Configure/build/run the standalone install consumer.
"""

from __future__ import annotations

import os
import pathlib
import shutil
import subprocess
import sys


ROOT = pathlib.Path(__file__).resolve().parents[1]
WORK = ROOT / ".release-check"
BUILD = WORK / "build"
BUILD_INSTALL = WORK / "build-install"
STAGE = WORK / "stage"
BUILD_CONSUME = WORK / "build-consume"


def run(cmd: list[str], cwd: pathlib.Path | None = None) -> None:
    print("+", " ".join(cmd))
    subprocess.run(cmd, cwd=str(cwd or ROOT), check=True)


def exe_path(build_dir: pathlib.Path, name: str) -> pathlib.Path:
    if os.name == "nt":
        return build_dir / "Release" / f"{name}.exe"
    return build_dir / name


def main() -> int:
    if WORK.exists():
        shutil.rmtree(WORK)
    WORK.mkdir(parents=True)

    run([sys.executable, "scripts/sync_docs_snippets.py"])

    run([
        "cmake",
        "-S",
        ".",
        "-B",
        str(BUILD),
        "-DRUDP_BUILD_TESTS=ON",
    ])
    run(["cmake", "--build", str(BUILD), "--config", "Release"])

    for exe in [
        "rudp_self_test",
        "rudp_reliability_test",
        "rudp_manager_test",
        "rudp_bench",
    ]:
        run([str(exe_path(BUILD, exe))])

    run([
        "cmake",
        "-S",
        ".",
        "-B",
        str(BUILD_INSTALL),
        "-DRUDP_INSTALL=ON",
        "-DRUDP_BUILD_TESTS=OFF",
        "-DRUDP_BUILD_EXAMPLES=OFF",
    ])
    run(["cmake", "--build", str(BUILD_INSTALL), "--config", "Release"])
    run([
        "cmake",
        "--install",
        str(BUILD_INSTALL),
        "--config",
        "Release",
        "--prefix",
        str(STAGE),
    ])

    run([
        "cmake",
        "-S",
        "tests/install_consume",
        "-B",
        str(BUILD_CONSUME),
        f"-DCMAKE_PREFIX_PATH={STAGE}",
    ])
    run(["cmake", "--build", str(BUILD_CONSUME), "--config", "Release"])
    run([str(exe_path(BUILD_CONSUME, "rudp_install_consume"))])

    print("release_check passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
