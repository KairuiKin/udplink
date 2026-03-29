# PR Draft: C ABI Stable Declaration Candidate

## Summary

This PR would intentionally move `udplink` from the current C ABI `wait` position to an explicitly stable v1 C ABI baseline.

It would do that in one coherent change set by:

- blessing `examples/c_api/install_consume/` as the first supported downstream baseline
- recording an explicit no-break expectation for the current v1 C ABI
- updating compatibility, assessment, release, and project-status docs together
- switching doc-state validation from `python scripts/check_c_abi_docs.py --mode wait` to `python scripts/check_c_abi_docs.py --mode stable`

## What changed

- updated `docs/c-abi-stability-assessment.md` from `not yet` to `ready`
- updated `docs/c-abi-downstream-baseline-decision.md` from `proposed` to `adopted`
- updated `docs/c-abi-compatibility.md` to state that the current v1 ABI is now treated as a stable baseline
- updated `docs/c-api-quickstart.md` to reflect a stable but still intentionally narrow C ABI surface
- updated `docs/project-status.md` to remove wait-only blocker language
- updated `docs/c-abi-stability-decision-playbook.md` and `docs/release-checklist.md` for stable-state operation
- updated `scripts/check_c_abi_docs.py` expectations for `--mode stable`
- kept the change set governance-focused rather than expanding the ABI surface itself

## Validation

- `python scripts/check_c_abi_docs.py --mode stable`
- `python scripts/release_check.py`
- `rudp_c_api_test`
- `tests/install_consume_c/`
- `examples/c_api/install_consume/`
- CI `docs-consistency`
- CI installed-package consumer validation on Linux and Windows

## Decision Basis

Maintainers are intentionally making both of the following statements together:

- `examples/c_api/install_consume/` is now the repository's first supported downstream baseline for the current C ABI
- maintainers do not expect a near-term breaking redesign of the current v1 C ABI baseline

## Risks / Notes

- this PR does not broaden the C ABI; it stabilizes the current narrow surface
- stable does not imply manager-layer C bindings, broader PlatformIO support, or broad board certification
- future ABI-sensitive changes should remain additive unless a new ABI revision is intentionally announced

## Follow-ups

- start any Rust wrapper work strictly on top of `rudp/rudp_c.h`
- keep board-backed validation as a separate embedded proof path, not part of the ABI stability claim itself
