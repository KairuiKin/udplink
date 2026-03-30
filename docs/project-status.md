# Project Status

Snapshot date: 2026-03-30.

This page is the short maintainer-facing read of where `udplink` stands right now, what is still unresolved, and what route should come next. Build, test, install, and API-entry details stay in `README.md` / `README.en.md`.

## Current Position

Current repository read:

- `v0.1.2` is already cut, and the core C++ library, install/export path, and release flow are in place.
- the minimal C ABI now exists in `include/rudp/rudp_c.h` with implementation in `src/rudp_c.cpp`.
- C ABI validation now spans `tests/c_api_test.cpp`, `tests/install_consume_c/`, and the maintained example in `examples/c_api/install_consume/`.
- Linux and Windows CI both exercise installed-package C++ and C consumers.
- embedded examples are aligned with the current API, but most are still reference templates rather than proven board paths.
- the first narrow board-backed route is drafted around Arduino Mega 2560 + W5100 + PlatformIO + `rudp_example_udp_peer`, with an execution checklist, prepared run pack, finalize helper, issue-filing helper, and bundle helper in `docs/arduino-mega-w5100-bringup.md` and `docs/arduino-mega-w5100-maintainer-run.md`.
- Rust work remains intentionally blocked until the ABI stability decision is finished.

## What Is Already Closed

The repository is no longer blocked on first-pass implementation work:

- core C++ build/test paths work on Windows, Linux, and macOS
- the minimal C ABI exists and is exercised by tests, installed-package consumers, and a maintained C example
- install/export metadata, docs consistency checks, and C ABI surface snapshot checks already exist
- the first narrow Arduino Mega 2560 + W5100 board path now has bring-up docs, a maintainer run pack, report finalization, issue-filing helpers, archive bundling, and CI-smoked evidence templates
- release-style local verification already ties docs checks, build/test, install/export, and installed-package consumer checks together

If you need build commands, installed-package usage, or API quick views, use `README.md` / `README.en.md`.

## What Is Still Open

The main unfinished items are now decision and validation items, not basic plumbing:

- the current C ABI is implemented, but it is not yet declared stable
- maintainers have not yet blessed `examples/c_api/install_consume/` as the first supported downstream baseline
- there is not yet an explicit no-break statement for the current v1 C ABI baseline
- the Arduino Mega 2560 + W5100 bring-up path is execution-ready on paper, but not yet hardware-confirmed
- broad PlatformIO, board-matrix, or Rust expansion should not start before the above items are resolved

## Recommended Route

Recommended order from here:

1. Keep the current validation baseline green.
   - Keep Windows build, test, install-consume, and release-style checks aligned.
   - Treat that path as the repository's strongest maintained validation route.
2. Confirm one real board-backed path.
   - Use `docs/arduino-mega-w5100-bringup.md` as the first candidate path.
   - Treat success here as a proof point for embedded usability, not as a promise for every board family.
3. Then decide whether the current C ABI should remain `wait` or intentionally move to `stable`.
   - Use `docs/c-abi-stability-gate.md`, `docs/c-abi-stability-assessment.md`, and `docs/c-abi-downstream-baseline-decision.md` together.
   - Either explicitly bless the maintained C consumer example and state that no near-term ABI break is expected, or keep the ABI in draft status on purpose.
4. Only after the ABI gate is intentionally closed, start a Rust wrapper spike.
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

### Official decision and validation pages

- `docs/c-abi-stability-gate.md`
- `docs/c-abi-stability-assessment.md`
- `docs/c-abi-downstream-baseline-decision.md`
- `docs/c-abi-stability-decision-playbook.md`
- `docs/c-abi-current-wait-decision-record.md`
- `docs/c-abi-stable-candidate-change-set.md`
- `docs/board-bringup-report-example.md`
- `docs/arduino-mega-w5100-bringup.md`
- `docs/arduino-mega-w5100-maintainer-run.md`
- `MAINTAIN.md`

### Draft and release records

- `docs/drafts-and-records.md`
