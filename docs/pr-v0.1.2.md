# PR Draft: v0.1.2 Stabilization

## Summary

This PR prepares `udplink` for `v0.1.2` by tightening release verification, example/API consistency, and package-consumer validation.

## What changed

- added deterministic reliability regression coverage with `rudp_reliability_test`
- added install/export validation with a standalone `find_package(rudp)` consumer
- added a one-command local release verification flow via `scripts/release_check.py`
- documented benchmark methodology and added benchmark smoke to CI
- aligned embedded examples with the current callback-based API
- documented API mapping to reduce future example drift
- removed unsafe raw object zeroing from manager initialization and added explicit endpoint state reset

## Validation

- `python scripts/release_check.py`
- `rudp_self_test`
- `rudp_reliability_test`
- `rudp_manager_test`
- `rudp_bench`
- `tests/install_consume`

## Risks / Notes

- Embedded board-specific compilation is still manual and remains outside the main CI matrix.
- This release is primarily about engineering quality and release confidence, not new wire-level features.

## Follow-ups

- expand board-specific embedded adaptation notes
- consider additional Unix warning cleanup if CI logs still show actionable noise
