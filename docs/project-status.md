# Project Status

Snapshot date: 2026-03-28.

This page is the short maintainer-facing read of where `udplink` stands right now, what is actually done, and what route should come next.

## Current Position

Current repository read:

- `v0.1.2` is already cut, and the core C++ library, install/export path, and release flow are in place.
- the minimal C ABI now exists in `include/rudp/rudp_c.h` with implementation in `src/rudp_c.cpp`.
- C ABI validation now spans `tests/c_api_test.cpp`, `tests/install_consume_c/`, and the maintained example in `examples/c_api/install_consume/`.
- Linux and Windows CI both exercise installed-package C++ and C consumers.
- embedded examples are aligned with the current API, but most are still reference templates rather than proven board paths.
- the first narrow board-backed route is drafted around Arduino Mega 2560 + W5100 + PlatformIO + `rudp_example_udp_peer`, with an execution checklist and evidence template in `docs/arduino-mega-w5100-bringup.md`.
- Rust work remains intentionally blocked until the ABI stability decision is finished.

## Recently Completed

Recent work already landed in the repository:

- minimal public C ABI header and wrapper
- C ABI smoke test and installed-package C smoke consumer
- maintained C API consumer example that builds against the installed package
- CI and `scripts/release_check.py` coverage for the C ABI and installed-package C consumption
- documentation for C ABI quickstart, compatibility, stability gate, current assessment, downstream-baseline decision, and decision playbook
- doc-state consistency checker in `scripts/check_c_abi_docs.py`, wired into local release verification and CI
- public C ABI surface snapshot in `docs/c-abi-surface-v1.json`, enforced by `scripts/check_c_abi_surface.py`
- gate-evaluation helper in `scripts/evaluate_c_abi_gate.py` to separate technical readiness from unresolved governance promises
- narrow PlatformIO scope definition instead of a broad unsupported matrix
- drafted board bring-up guide for Arduino Mega 2560 + W5100, now structured as an execution checklist with acceptance criteria
- maintainer-run pack and host-peer log capture helper for board-backed validation attempts
- board-run workspace initializer that generates summary, serial-notes, and report-draft files
- issue templates for board-backed validation reports and C ABI stability decisions
- pull request template and contributing guide updated to reflect the current C ABI validation and governance flow
- host-side UDP peer example wired into CMake as `rudp_example_udp_peer`
- `ConnectionManager::Find()` repositioned as compatibility-only rather than a preferred new-code path

## What Is Still Open

The main unfinished items are now decision and validation items, not basic plumbing:

- the current C ABI is implemented, but it is not yet declared stable
- maintainers have not yet blessed `examples/c_api/install_consume/` as the first supported downstream baseline
- there is not yet an explicit no-break statement for the current v1 C ABI baseline
- the Arduino Mega 2560 + W5100 bring-up path is execution-ready on paper, but not yet hardware-confirmed
- broad PlatformIO, board-matrix, or Rust expansion should not start before the above items are resolved

## Recommended Route

Recommended order from here:

1. Finish the C ABI stability decision.
   - Use `docs/c-abi-stability-gate.md`, `docs/c-abi-stability-assessment.md`, and `docs/c-abi-downstream-baseline-decision.md` together.
   - Either explicitly bless the maintained C consumer example and state that no near-term ABI break is expected, or keep the ABI in draft status on purpose.
2. Confirm one real board-backed path.
   - Use `docs/arduino-mega-w5100-bringup.md` as the first candidate path.
   - Treat success here as a proof point for embedded usability, not as a promise for every board family.
3. Only after the ABI gate is met, start a Rust wrapper spike.
   - The wrapper must stay strictly on top of `rudp/rudp_c.h`.
   - Any missing pieces should feed back as focused ABI additions instead of bypassing the C layer.

## What Not To Do Next

Avoid these until the current route is closed:

- expanding the C ABI just to make it feel more complete
- starting manager-level C bindings without a real downstream need
- broadening PlatformIO support into a board matrix without maintainable validation
- starting Rust work before the ABI support promise is explicit

## Decision Links

Use these pages as the authoritative route map:

- `docs/c-abi-stability-gate.md`
- `docs/c-abi-stability-assessment.md`
- `docs/c-abi-downstream-baseline-decision.md`
- `docs/c-abi-stability-decision-playbook.md`
- `docs/c-abi-current-wait-decision-record.md`
- `docs/c-abi-stable-candidate-change-set.md`
- `docs/c-abi-stable-wording-pack.md`
- `docs/pr-c-abi-stable-candidate.md`
- `docs/c-abi-stable-patch-draft.md`
- `docs/board-bringup-report-example.md`
- `docs/arduino-mega-w5100-bringup.md`
- `docs/arduino-mega-w5100-maintainer-run.md`
- `docs/issue-drafts-post-v0.1.2.md`
- `MAINTAIN.md`
