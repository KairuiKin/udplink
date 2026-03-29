# C ABI Downstream Baseline Decision

## Purpose

This document records the maintainer-facing decision about whether `examples/c_api/install_consume/` should count as the first supported downstream baseline for the current C ABI.

This is not the same as declaring the ABI stable.
It answers the narrower policy question that now sits between the current repository state and any future stability claim.

## Current Status

Decision status: proposed, not yet adopted.

Current recommendation: do not bless it yet.

Reason:

- the example is now maintained and validated through CI and `scripts/release_check.py`
- but maintainers still have not explicitly accepted the support promise that comes with treating it as the downstream baseline
- stability declarations should follow that promise, not imply it accidentally

## What "Bless" Means Here

If maintainers bless `examples/c_api/install_consume/` as the downstream baseline, the repository is making all of the following promises together:

- the example is not just a demo; it is a supported reference consumer for the current C ABI
- ABI-sensitive changes should keep this example working unless a deliberate ABI revision is announced
- CI and release-check failures in this example are treated as meaningful compatibility regressions, not optional breakage
- release and maintain docs may point to this example as the concrete consumer story that helps close the downstream-use-case gate

## Option A: Bless It

Choose this option only if maintainers are comfortable saying:

- `examples/c_api/install_consume/` is the repository's first supported downstream baseline
- the current v1 C ABI is not expected to need a near-term breaking redesign for this example to remain valid
- keeping this example working is part of normal release discipline

Implications:

- the repository moves materially closer to a defensible first stability declaration
- release docs can use this example as the concrete downstream-consumer story
- future ABI discussions should assume this example carries real compatibility weight

## Option B: Do Not Bless It Yet

Choose this option if maintainers still want the example to remain evidence, but not yet a support promise.

Implications:

- the example remains useful validation and documentation
- the repository should keep the current v1 ABI unchanged where practical
- the current recommendation stays `wait`, not `stable`
- a future external consumer, stronger in-repo commitment, or more explicit no-break position would still be needed before making a stability claim

## Current Recommendation

Recommendation today: choose Option B for now.

Why:

- technical validation is already strong enough to support the example as evidence
- the remaining gap is governance, not implementation wiring
- there is still no explicit maintainer statement that this example now carries compatibility weight comparable to a supported downstream baseline
- there is still no explicit release-facing no-break confirmation for the current v1 C ABI

## Conditions To Revisit

Maintainers should revisit this decision when all of the following feel true together:

- they are willing to call `examples/c_api/install_consume/` a supported reference consumer in release-facing docs
- they are willing to keep the current v1 ABI working for that example absent a deliberate ABI revision
- they are willing to state that no near-term ABI break is expected for the current baseline

If those conditions are met, the next reasonable step is to update this page from proposed to adopted and then reassess whether the C ABI may be declared stable.

## Related Documents

- `docs/c-abi-stability-gate.md`
- `docs/c-abi-stability-assessment.md`
- `docs/c-api-quickstart.md`
- `docs/release-checklist.md`
- `MAINTAIN.md`