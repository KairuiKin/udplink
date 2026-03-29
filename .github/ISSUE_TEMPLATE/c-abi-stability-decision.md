---
name: C ABI Stability Decision
about: Record a maintainer decision to keep wait or declare the current C ABI stable.
title: "abi: decision for current C ABI baseline"
labels: ["api", "decision"]
assignees: []
---

## Decision

Choose one:

- keep `wait`
- declare `stable`

## Decision Inputs Reviewed

- [ ] `docs/c-abi-stability-gate.md`
- [ ] `docs/c-abi-stability-assessment.md`
- [ ] `docs/c-abi-downstream-baseline-decision.md`
- [ ] `docs/c-abi-stability-decision-playbook.md`
- [ ] `docs/c-abi-stable-candidate-change-set.md`
- [ ] `docs/c-abi-stable-wording-pack.md`

## Downstream Baseline Decision

State whether maintainers bless `examples/c_api/install_consume/` as the first supported downstream baseline.

## No-Break Position

State whether maintainers explicitly expect no near-term breaking redesign for the current v1 C ABI.

## Validation Status

- `python scripts/check_c_abi_docs.py --mode wait|stable`:
- `python scripts/release_check.py`:
- `rudp_c_api_test`:
- `tests/install_consume_c/`:
- `examples/c_api/install_consume/`:

## Required Follow-Through

If the decision is `wait`:

- [ ] official docs remain in wait state
- [ ] `python scripts/check_c_abi_docs.py --mode wait` passes
- [ ] no stable-only wording is merged into official docs

If the decision is `stable`:

- [ ] official docs are updated in one change set
- [ ] `python scripts/check_c_abi_docs.py --mode stable` passes
- [ ] no wait-only wording remains in official stable docs
- [ ] release notes and changelog include the stable declaration intentionally

## Notes

Add any maintainer rationale, caveats, or follow-up tasks.
