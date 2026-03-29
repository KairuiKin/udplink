# C ABI Stability Decision Playbook

Snapshot date: 2026-03-28.

This page is the maintainer playbook for making the next explicit C ABI decision without drifting between `wait` and `stable` language.

Use it together with:

- `docs/c-abi-stability-gate.md`
- `docs/c-abi-stability-assessment.md`
- `docs/c-abi-downstream-baseline-decision.md`
- `docs/release-checklist.md`
- `docs/c-abi-surface-v1.json`
- `scripts/check_c_abi_surface.py`
- `docs/c-abi-stable-candidate-change-set.md`

## Purpose

This page answers three practical questions:

1. what exactly must maintainers decide now?
2. what should the repository say if the answer is still `wait`?
3. what exact statements and follow-up edits are required if the answer becomes `stable`?

## Decision To Make

There are really two linked decisions, and both must point in the same direction before any stability declaration:

1. Baseline policy decision
   - do maintainers bless `examples/c_api/install_consume/` as the first supported downstream baseline?
2. Stability declaration decision
   - do maintainers explicitly state that the current v1 C ABI is not expected to need a near-term breaking change?

If either answer is still no or unclear, the repository should stay on `wait`.

## Current Recommended Answer

Current recommended answer: keep `wait`.

Reason:

- the technical validation path is now strong enough, including a checked-in v1 surface snapshot
- the remaining gap is an explicit support promise and no-break statement
- those promises should be made intentionally, not inferred from tests alone

## Path A: Keep Wait

Choose this path if maintainers are not yet ready to make both promises above.

### Repository Position

Use language equivalent to all of the following together:

- the minimal C ABI is implemented
- the current C ABI remains in draft status
- `examples/c_api/install_consume/` is maintained evidence, but not yet the adopted supported downstream baseline
- Rust and other new FFI work remain blocked on the stability decision
- the current v1 ABI should still be preserved where practical even without a stability claim

### Required Follow-Through

If maintainers keep `wait`, make sure all of the following remain true:

- `python scripts/check_c_abi_docs.py --mode wait` still passes
- `python scripts/check_c_abi_surface.py` still passes
- no stable-only wording has leaked into the official wait-state docs
- `docs/c-abi-stability-assessment.md` still says `not yet`
- `docs/c-abi-downstream-baseline-decision.md` stays `proposed, not yet adopted`
- `docs/project-status.md` keeps the open blocker language
- release notes do not claim the C ABI is stable
- new C ABI changes remain conservative and should avoid avoidable churn

### Suggested Release-Facing Wording

Use wording close to this if a release wants to mention the C ABI without declaring stability:

> The repository ships a minimal public C ABI with CI-backed validation, a checked-in v1 surface snapshot, and installed-package consumption, but it is not yet declared a long-term stable baseline.

## Path B: Declare Stable

Choose this path only if maintainers are ready to do both of the following intentionally:

- bless `examples/c_api/install_consume/` as the first supported downstream baseline
- state that no near-term breaking redesign is expected for the current v1 C ABI

### Preconditions Before Declaring Stable

Do not proceed unless all of the following are true together:

- every item in `docs/c-abi-stability-gate.md` is satisfied
- `python scripts/check_c_abi_surface.py` passes on the release candidate
- `rudp_c_api_test` passes on the release candidate
- `tests/install_consume_c/` passes on the release candidate
- `examples/c_api/install_consume/` passes on the release candidate
- `python scripts/release_check.py` passes on the release candidate
- maintainers are willing to treat regressions in the maintained C consumer example as real compatibility regressions

### Required Repository Updates

If maintainers choose `stable`, update all of the following in the same change set:

1. `docs/c-abi-downstream-baseline-decision.md`
   - switch from `proposed, not yet adopted` to an adopted state
   - replace the current recommendation with the adopted baseline decision
2. `docs/c-abi-stability-assessment.md`
   - change the current answer from `not yet` to `ready` or equivalent release-candidate wording
   - record the explicit no-break confirmation
3. `docs/c-abi-compatibility.md`
   - state that the current v1 ABI is now an explicitly stable baseline
4. `docs/project-status.md`
   - remove the open blocker language that says the ABI is not yet stable
5. release notes and changelog
   - say clearly that the current C ABI baseline is now considered stable

If those edits do not happen together, the repository is still effectively in `wait` mode.

### Suggested Stable Declaration Text

Use wording close to this only when the above preconditions and edits are complete:

> The current v1 `udplink` C ABI is now treated as a stable baseline. The repository's maintained installed-package C consumer example in `examples/c_api/install_consume/` is the first supported downstream reference consumer for that ABI.

### Suggested No-Break Confirmation Text

Maintain docs should also contain language close to this:

> Maintainers do not expect a near-term breaking redesign of the current v1 C ABI. Future ABI-sensitive changes should therefore remain additive unless a new ABI revision is intentionally announced.

## Files To Check Before Merging Either Decision

Review these pages together before merging the decision change:

- `docs/c-abi-stability-gate.md`
- `docs/c-abi-stability-assessment.md`
- `docs/c-abi-downstream-baseline-decision.md`
- `docs/c-abi-compatibility.md`
- `docs/project-status.md`
- `docs/release-checklist.md`
- `MAINTAIN.md`

## What Not To Do

Avoid these failure modes:

- declaring the ABI stable in release notes while the assessment page still says `not yet`
- blessing the downstream baseline implicitly without recording that policy decision
- keeping the ABI labeled `draft` after maintainers already made a stable support promise
- expanding the ABI first and postponing the actual decision again

## Practical Recommendation

The repository is now close enough that the next useful maintainer step is not more generic planning.

It is one of these two concrete actions:

1. keep `wait` intentionally and preserve the current baseline unchanged
2. adopt the downstream baseline and make the stability declaration in one deliberate release-facing change set
