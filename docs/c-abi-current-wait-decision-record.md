# Current C ABI Wait Decision Record

Snapshot date: 2026-03-30.

This page records the repository's current explicit C ABI answer: keep `wait`.

It does not declare the ABI stable.
It records why `wait` is still the correct repository answer even after the post-`v0.1.2` hardening work made the technical side much stronger.

Use it together with:

- `docs/c-abi-stability-gate.md`
- `docs/c-abi-stability-assessment.md`
- `docs/c-abi-downstream-baseline-decision.md`
- `docs/c-abi-stability-decision-playbook.md`
- `scripts/evaluate_c_abi_gate.py`

## Current Answer

Current repository answer: keep `wait`.

Repository position expressed plainly:

- the minimal C ABI is implemented and intentionally preserved
- the current v1 ABI is still not declared a long-term stable baseline
- `examples/c_api/install_consume/` remains maintained evidence, but not yet the formally adopted downstream baseline
- Rust and other new FFI work remain blocked on an intentional stability decision

## Why `wait` Is Still Correct

The technical side is now much stronger than before:

- the public C header and wrapper exist
- installed-package C and C++ consumers run in CI
- `scripts/release_check.py` exercises the maintained C consumer path
- `scripts/check_c_abi_surface.py` pins the checked-in v1 public surface
- `scripts/evaluate_c_abi_gate.py --docs-mode wait` reports that the main technical gate areas are already met or effectively met

The remaining blockers are governance blockers, not plumbing blockers:

1. maintainers have still not explicitly blessed `examples/c_api/install_consume/` as the first supported downstream baseline
2. maintainers have still not made the release-facing statement that no near-term breaking redesign is expected for the current v1 ABI

Until both statements are intentionally made, the safe repository answer remains `wait`.

## Inputs Reviewed

This decision record assumes the current repository state has been reviewed through at least:

- `python scripts/check_c_abi_docs.py --mode wait`
- `python scripts/check_c_abi_surface.py`
- `python scripts/evaluate_c_abi_gate.py --docs-mode wait`
- `python scripts/release_check.py`

At the time of this record, the expected gate read is:

- gates 1-4: met or effectively met for the current repository state
- gate 5: still open pending an adopted downstream-baseline promise
- gate 6: still open pending an explicit no-near-term-break confirmation

## Maintainer Wording

If maintainers want to record the current answer without declaring stability, wording close to the following is appropriate:

> The repository now ships a minimal public C ABI with CI-backed validation, installed-package consumers, and a checked-in v1 surface snapshot. However, maintainers are intentionally keeping the current C ABI in `wait` state because the downstream baseline has not yet been formally adopted and the repository has not yet made an explicit no-near-term-break support promise for the current v1 ABI.

## Consequences Of Keeping `wait`

If the repository keeps `wait`, all of the following should remain true:

- docs stay aligned on `implemented but not yet declared stable`
- `python scripts/check_c_abi_docs.py --mode wait` keeps passing
- `python scripts/check_c_abi_surface.py` keeps passing
- `python scripts/evaluate_c_abi_gate.py --docs-mode wait` continues to report governance blockers rather than hidden technical drift
- the current v1 ABI is preserved conservatively even without a formal stability claim
- Rust work stays blocked on top of the current C ABI until the decision changes intentionally

## What `wait` Does Not Mean

Keeping `wait` does not mean:

- the current C ABI is low quality
- the repository lacks validation structure
- more generic process documents are needed before a decision can be made
- maintainers must wait for a large external ecosystem before ever calling the ABI stable

It means only that the remaining promises should be made deliberately rather than inferred from tests and examples.

## Smallest Path To Change This Answer

To move from `wait` toward `stable`, maintainers would need to do all of the following together:

1. adopt `examples/c_api/install_consume/` as the first supported downstream baseline
2. write the explicit no-near-term-break confirmation for the current v1 ABI
3. update docs into stable mode in one coherent change set
4. make `python scripts/evaluate_c_abi_gate.py --docs-mode stable --downstream-baseline-adopted --no-near-term-break --require-stable-ready` pass on that candidate branch

## Relation To Issue #10

Issue `#10` asks whether the current C ABI baseline should now be declared stable.

The current explicit repository answer is still no.
The reason is no longer missing technical structure; it is the absence of two deliberate maintainer promises:

- bless the downstream baseline
- make the no-near-term-break statement
