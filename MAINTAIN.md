# Maintain Notes

This file records the repository's current maintainer-facing operating state.
It is intentionally shorter than the full docs tree and should answer three questions quickly:

1. What is shipped today?
2. What is verified today?
3. What should maintainers push next?

## Current shipped scope

- C++ protocol core: shipped
- connection manager layer: shipped
- minimal C ABI: shipped
- install/export metadata: shipped
- maintained C installed-package example: shipped
- narrow PlatformIO/Arduino local path: shipped as a reference path, not as a broad support promise
- Rust bindings: deferred

## Current verification status

### Windows

Verified locally in this repository:

- main `Release` build
- `ctest --test-dir build -C Release --output-on-failure`
- `tests/install_consume`
- `tests/install_consume_c`
- `examples/c_api/install_consume`
- host-side `rudp_example_udp_peer`
- `scripts/prepare_mega_w5100_run.ps1`

Windows is currently the strongest end-to-end validation path in the repository.

### Unix-like CI

Covered in CI:

- core build on Ubuntu/macOS
- smoke/reliability/manager/C ABI tests
- install-consume checks on Linux
- embedded template compile target

### Board-backed validation

Status today:

- documented
- locally buildable
- operator workflow prepared
- not yet confirmed with real hardware evidence in this repository

The first narrow board-backed route remains:

- Arduino Mega 2560
- W5100-compatible shield
- PlatformIO
- `rudp_example_udp_peer`

## C ABI position

Current repository position: `wait`

That means all of the following remain true:

- the C ABI is implemented
- the C ABI is exercised by tests and installed-package consumers
- the C ABI is not yet declared long-term stable
- maintainers have not yet blessed `examples/c_api/install_consume/` as the first supported downstream baseline

Do not silently drift docs or release wording away from that state.
If maintainers want to change that answer, use:

- `docs/c-abi-stability-gate.md`
- `docs/c-abi-stability-assessment.md`
- `docs/c-abi-downstream-baseline-decision.md`
- `docs/c-abi-stability-decision-playbook.md`
- `scripts/check_c_abi_docs.py`

## Validation entry points maintainers should prefer

### Core validation

- `ctest --test-dir build -C Release --output-on-failure` on Windows
- `ctest --test-dir build --output-on-failure` on Unix-like systems
- `python scripts/release_check.py` for a release-style local pass

### Install/export validation

Preferred rule:

- Unix-like consumers may use `CMAKE_PREFIX_PATH`
- Windows consumers should prefer explicit `rudp_DIR:PATH=<stage>\lib\cmake\rudp`

This is now the documented and CI-aligned Windows path.

### Board-run preparation

Use:

- `scripts/prepare_mega_w5100_run.ps1` on Windows for the fast-path run pack
- `scripts/init_board_run.py` if only the run directory templates are needed
- `scripts/run_peer_capture.py` to tee host peer output to a log

## Current priorities

### P1

- keep Windows build/test/install-consume docs, scripts, and CI aligned
- reduce duplicate or contradictory wording across README, docs index, contributing guide, and release checklist
- preserve the current C ABI `wait` wording until maintainers intentionally change it

### P2

- get one real board-backed Arduino Mega 2560 + W5100 evidence run into the repository workflow
- keep PlatformIO narrow; do not expand into a broad support matrix without real demand and validation
- extend the C ABI only when there is a concrete downstream need

### P3

- revisit Rust only after the C ABI stability decision is intentionally closed
- revisit true deprecation strategy for `ConnectionManager::Find()` only in a deliberate breaking-change window

## What maintainers should avoid

- do not claim the current C ABI is stable unless the stability gate and wording pack are intentionally updated together
- do not treat the Arduino/PlatformIO path as proof of broad embedded support
- do not let Windows instructions drift back to hard-coded Visual Studio generator assumptions
- do not add new consumer-facing surface area without matching tests and installed-package validation

## Practical next route

If the goal is repository hardening rather than new feature spread, the next highest-value sequence is:

1. keep Windows validation green
2. keep install-consume paths green
3. reduce doc drift and duplication
4. obtain one real board-backed evidence run
5. only then decide whether the C ABI should stay `wait` or move to `stable`
