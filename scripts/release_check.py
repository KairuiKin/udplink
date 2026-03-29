#!/usr/bin/env python3
"""Run the local release verification flow for udplink.

This script performs the same core checks that the project now expects before
cutting a release:

1. Sync docs snippets.
2. Check C ABI doc-state consistency and the v1 public surface baseline.
3. Configure/build the main project in Release mode.
4. Run self/reliability/manager/C ABI/bench executables.
5. Build and install the package.
6. Configure/build/run the standalone install consumers and maintained C API consumer example.
"""

from __future__ import annotations

import platform
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / "build"
BUILD_INSTALL = ROOT / "build-install"
BUILD_CONSUME = ROOT / "build-consume"
BUILD_CONSUME_C = ROOT / "build-consume-c"
BUILD_EXAMPLE_C = ROOT / "build-example-c-api"
STAGE = ROOT / "stage"
RUDP_DIR = STAGE / "lib" / "cmake" / "rudp"


def run(cmd: list[str]) -> None:
    print("+", " ".join(cmd))
    subprocess.run(cmd, check=True, cwd=ROOT)


def exe_path(build_dir: Path, name: str) -> Path:
    if platform.system() == "Windows":
        return build_dir / "Release" / f"{name}.exe"
    return build_dir / name


def main() -> int:
    run(["python", "scripts/sync_docs_snippets.py"])
    run(["python", "scripts/check_c_abi_docs.py", "--mode", "wait"])
    run(["python", "scripts/check_c_abi_surface.py"])

    run([
        "cmake",
        "-S",
        ".",
        "-B",
        str(BUILD),
        "-DRUDP_BUILD_TESTS=ON",
        "-DRUDP_BUILD_EXAMPLES=ON",
    ])
    run(["cmake", "--build", str(BUILD), "--config", "Release"])

    run([str(exe_path(BUILD, "rudp_self_test"))])
    run([str(exe_path(BUILD, "rudp_reliability_test"))])
    run([str(exe_path(BUILD, "rudp_manager_test"))])
    run([str(exe_path(BUILD, "rudp_c_api_test"))])
    run([str(exe_path(BUILD, "rudp_bench"))])

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
        f"-Drudp_DIR:PATH={RUDP_DIR}",
    ])
    run(["cmake", "--build", str(BUILD_CONSUME), "--config", "Release"])
    run([str(exe_path(BUILD_CONSUME, "rudp_install_consume"))])

    run([
        "cmake",
        "-S",
        "tests/install_consume_c",
        "-B",
        str(BUILD_CONSUME_C),
        f"-Drudp_DIR:PATH={RUDP_DIR}",
    ])
    run(["cmake", "--build", str(BUILD_CONSUME_C), "--config", "Release"])
    run([str(exe_path(BUILD_CONSUME_C, "rudp_install_consume_c"))])

    run([
        "cmake",
        "-S",
        "examples/c_api/install_consume",
        "-B",
        str(BUILD_EXAMPLE_C),
        f"-Drudp_DIR:PATH={RUDP_DIR}",
    ])
    run(["cmake", "--build", str(BUILD_EXAMPLE_C), "--config", "Release"])
    run([str(exe_path(BUILD_EXAMPLE_C, "rudp_c_api_example"))])

    print("release_check passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
