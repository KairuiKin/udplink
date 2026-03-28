# v0.1.2 Release Plan

Target cut date: `2026-03-28`

`v0.1.2` is a stabilization release focused on consistency, validation depth, and packaging confidence.

## Delivered in this cycle

- Added deterministic loss/reorder regression coverage via `rudp_reliability_test`.
- Kept `rudp_self_test` focused on stable smoke scenarios.
- Updated embedded examples to the current `rudp::Hooks` / `Endpoint::StartConnect()` API.
- Added install/export validation with a standalone `find_package(rudp)` consumer.
- Added benchmark methodology and a local sample result page.
- Added an API mapping page for keeping examples aligned with the current public API.
- Added a one-command local release verification script.
- Removed unsafe object-level zeroing from `ConnectionManager` initialization.
- Added explicit endpoint state reset flow for re-init safety.

## Release Notes Draft

### Added

- `rudp_reliability_test` for deterministic reliability regression coverage.
- `tests/install_consume` smoke consumer for package install verification.
- CI `install-consume` job to validate exported CMake package usage.
- Benchmark smoke execution in CI plus a benchmark methodology page.
- API mapping page to keep example integrations aligned.
- `scripts/release_check.py` for repeatable local release verification.

### Changed

- Embedded examples now match the current public API and are documented as reference templates.
- Documentation now distinguishes smoke tests from reliability regression coverage.
- Endpoint initialization now resets protocol state explicitly instead of depending on raw object zeroing.

### Fixed

- `ConnectionManager` no longer zeroes endpoint-containing slots as raw memory during construction.
- Re-initialization now clears endpoint runtime state before applying fresh config and hooks.

### Impact

- Lower risk of documentation drift around examples and tests.
- Better confidence that package consumers can use the exported target directly.
- Better visibility into protocol-core performance regressions.
- Safer connection reuse and manager slot lifecycle behavior.

## Exit Criteria

- All CI jobs pass, including `install-consume`.
- README, changelog, and maintain docs remain consistent.
- No known regression in `self_test`, `reliability_test`, `manager_test`, or install consume path.

## Known Follow-ups

- Board-specific embedded build validation is still manual.
- Additional Unix warning cleanup may still be worth a dedicated pass.

## GitHub Release Draft

`v0.1.2` focuses on release engineering and integration stability rather than new protocol surface area.

Highlights:

- adds deterministic reliability regression coverage via `rudp_reliability_test`
- validates install/export with a standalone `find_package(rudp)` consumer in CI
- documents and runs benchmark smoke in CI
- aligns embedded examples with the current callback-based API
- hardens endpoint/manager initialization against stale state and unsafe object zeroing

Recommended validation before tagging:

- run `python scripts/release_check.py`
- confirm CI passes including `install-consume`
- confirm the `0.1.2 - 2026-03-28` changelog section matches the final merged state

Operational docs:

- PR body draft: `docs/pr-v0.1.2.md`
- release cut sequence: `docs/release-cut-v0.1.2.md`
