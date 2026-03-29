# C ABI Stable Candidate Change Set

Snapshot date: 2026-03-28.

This page is a draft-only helper for the future case where maintainers decide to flip the repository from `wait` to `stable`.

It does not change the current repository position by itself.
Current official position remains `wait` until the corresponding docs are actually updated.

## Purpose

Use this page when maintainers are ready to make one deliberate stability declaration change set instead of editing every file ad hoc.

## Preconditions

Do not use this draft unless all of the following are already true:

- maintainers are ready to bless `examples/c_api/install_consume/` as the first supported downstream baseline
- maintainers are ready to state that no near-term breaking redesign is expected for the current v1 C ABI
- ABI-sensitive validation is green on the release candidate
- `python scripts/check_c_abi_docs.py --mode stable` has been updated to pass with the final stable wording and to reject leftover wait-only wording

## Candidate File Updates

Use `docs/c-abi-stable-wording-pack.md` as the exact wording source for the edits below.


### `docs/c-abi-stability-assessment.md`

Target wording:

- `Current answer: ready.`
- `Recommendation: the current C ABI baseline may be declared stable.`
- add an explicit no-break confirmation for the current v1 ABI

### `docs/c-abi-downstream-baseline-decision.md`

Target wording:

- `Decision status: adopted.`
- `Current recommendation: bless it.`
- state that `examples/c_api/install_consume/` is now the first supported downstream baseline

### `docs/c-abi-compatibility.md`

Target wording:

- `the current v1 ABI is now treated as an explicitly stable baseline`
- future ABI-sensitive changes stay additive unless a new ABI revision is announced

### `docs/c-api-quickstart.md`

Target wording:

- replace the current not-yet-stable limit with wording that the current C ABI baseline is now treated as stable
- keep the surface small; stable does not mean broad

### `docs/project-status.md`

Target wording:

- replace `the current C ABI is implemented, but it is not yet declared stable`
- replace `maintainers have not yet blessed ...`
- state that the baseline is stable and the maintained C consumer example is the first supported downstream baseline

### `docs/c-abi-stability-decision-playbook.md`

Target wording:

- `Current recommended answer: declare `stable`.`
- `python scripts/check_c_abi_docs.py --mode stable`
- keep Path A and Path B for historical clarity, but make the top-level recommendation reflect the new state

### `docs/release-checklist.md`

Target wording:

- replace `python scripts/check_c_abi_docs.py --mode wait`
- use `python scripts/check_c_abi_docs.py --mode stable`
- keep the explicit stable-claim release cut step

## Candidate Stable Declaration Text

Use wording close to this in release-facing docs:

> The current v1 `udplink` C ABI is now treated as a stable baseline. The maintained installed-package C consumer in `examples/c_api/install_consume/` is the repository's first supported downstream reference consumer for that ABI.

## Candidate No-Break Confirmation Text

Use wording close to this in maintain docs:

> Maintainers do not expect a near-term breaking redesign of the current v1 C ABI. Future ABI-sensitive changes should therefore remain additive unless a new ABI revision is intentionally announced.

## Merge Rule

Do not merge only part of this draft.

If maintainers choose `stable`, the decision should land as one coherent change set across the official docs, release wording, and `scripts/check_c_abi_docs.py --mode stable`.
