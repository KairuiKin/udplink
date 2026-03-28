# GitHub Release Draft: v0.1.2

`v0.1.2` is a stabilization release focused on verification depth, packaging confidence, and example/documentation consistency.

## Highlights

- Added deterministic reliability regression coverage with `rudp_reliability_test`.
- Added install/export validation with a standalone `find_package(rudp)` consumer in CI.
- Added a one-command local release verification flow via `python scripts/release_check.py`.
- Documented benchmark methodology and added benchmark smoke execution in CI.
- Updated embedded examples to the current callback-based API and documented them as reference templates.
- Hardened endpoint and manager initialization to avoid stale state and unsafe raw object zeroing.

## Validation

- `rudp_self_test`
- `rudp_reliability_test`
- `rudp_manager_test`
- `rudp_bench`
- `tests/install_consume`

## Notes

- This release improves release engineering and integration safety; it does not introduce a large new protocol feature set.
- Board-specific embedded compilation remains a manual validation path.
