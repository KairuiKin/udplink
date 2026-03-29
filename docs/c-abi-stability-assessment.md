# C ABI Stability Assessment

## Purpose

This page records the current repository assessment against `docs/c-abi-stability-gate.md`.

It is not a release note and it is not a stability declaration.
Its job is to answer a narrower question:

- if maintainers had to decide today, is the current C ABI ready to be declared stable?

## Current Assessment

Current answer: not yet.

Reason:

- the repository now has a small implemented ABI, written compatibility rules, a checked-in public surface snapshot, CI-backed install-consume coverage, a maintained in-repo C consumer example, and aligned documentation
- but it still does not clearly satisfy the gate requirement for at least one credible downstream consumer story
- maintainers also have not yet written an explicit "no near-term ABI break expected" confirmation tied to a release candidate

That means the current ABI is much closer to stability than before, but the safe position is still to keep it implemented without making a formal long-term stability promise.

## Gate Review

### 1. Surface discipline is still intact

Status: met.

Evidence:

- the public C ABI remains centered on endpoint lifecycle, send/receive, tick, state, stats, and metrics
- manager-layer C bindings have not been added
- the ABI still relies on opaque handles and simple callback hooks instead of exposing C++ layout details

Relevant files:

- `include/rudp/rudp_c.h`
- `src/rudp_c.cpp`
- `docs/c-abi-draft.md`

### 2. ABI-sensitive validation passes consistently

Status: likely met on the configured validation path.

Evidence:

- `tests/c_api_test.cpp` exists and is wired into the normal test build
- `tests/install_consume_c/` validates installed-package C consumption
- `examples/c_api/install_consume/` is now part of the maintained validation path
- `scripts/check_c_abi_surface.py` now compares `include/rudp/rudp_c.h` against the checked-in v1 snapshot in `docs/c-abi-surface-v1.json`
- `scripts/release_check.py` includes the C ABI path, the public surface snapshot check, and the maintained C consumer example
- CI now runs the surface snapshot check plus C installed-package consumption and the maintained C consumer example on both Linux and Windows

Limit:

- this is still an engineering reading of repository wiring, not the same thing as a fresh green CI run attached to this document

Relevant files:

- `tests/c_api_test.cpp`
- `tests/install_consume_c/`
- `examples/c_api/install_consume/`
- `docs/c-abi-surface-v1.json`
- `scripts/check_c_abi_surface.py`
- `scripts/release_check.py`
- `.github/workflows/ci.yml`

### 3. Install and packaging shape are no longer in flux

Status: likely met.

Evidence:

- `include/rudp/rudp_c.h` is installed with the public headers
- installed-package C and C++ consumers both use exported package metadata instead of private include tricks
- the install path is exercised in both local release-check flow and CI configuration

Limit:

- broader host/toolchain coverage could still uncover packaging edge cases later

Relevant files:

- `CMakeLists.txt`
- `tests/install_consume/`
- `tests/install_consume_c/`
- `examples/c_api/install_consume/`
- `scripts/release_check.py`
- `.github/workflows/ci.yml`

### 4. Documentation agrees on the contract

Status: met.

Evidence:

- quickstart, draft, compatibility policy, stability gate, release checklist, maintain plan, and post-release backlog now describe the same high-level position
- the shared position is: implemented ABI, not yet formally declared stable

Relevant files:

- `docs/c-api-quickstart.md`
- `docs/c-abi-draft.md`
- `docs/c-abi-compatibility.md`
- `docs/c-abi-stability-gate.md`
- `docs/release-checklist.md`
- `MAINTAIN.md`
- `docs/issue-drafts-post-v0.1.2.md`

### 5. There is at least one credible downstream use case

Status: not yet met.

Why not yet:

- `tests/install_consume_c/` proves packaging and public-header usability, but it is still a repository-owned smoke consumer
- `examples/c_api/install_consume/` is now a maintained in-repo C consumer example, which narrows the gap but still does not yet amount to a clearly supported downstream integration story by itself
- there is still no maintained external C sample, real downstream integration, or wrapper spike proposal that clearly commits to the current ABI surface
- the project still says Rust work should wait until the ABI is stable, so Rust cannot yet count as the validating downstream consumer story

What would satisfy this gate:

- treating the new maintained C consumer example as an explicitly supported downstream baseline
- a concrete Rust wrapper spike proposal that stays fully inside `rudp_c.h`
- another FFI consumer path that exercises the current surface in a way maintainers are willing to support

Relevant files:

- `tests/install_consume_c/`
- `examples/c_api/install_consume/`
- `docs/platformio-rust-scope.md`
- `docs/issue-drafts-post-v0.1.2.md`

### 6. No known near-term breaking change is queued

Status: not yet explicitly confirmed.

Current read:

- there is no obvious document saying maintainers already plan to break the current v1 C ABI soon
- but there is also no explicit release-facing statement that the current baseline is considered good enough to freeze without near-term breakage

What is missing:

- a short maintainer-level confirmation, tied to a release decision, that no near-term ABI break is expected for the currently exported v1 surface

Relevant files:

- `docs/c-abi-compatibility.md`
- `docs/c-abi-stability-gate.md`
- `MAINTAIN.md`

## When The In-Repo C Consumer Example Is Enough

That question is now intentionally split from the technical assessment and tracked in `docs/c-abi-downstream-baseline-decision.md`.

This page keeps the engineering read simple:

- `examples/c_api/install_consume/` is strong evidence that the current ABI is consumable
- it should only count as the gate-closing downstream baseline if maintainers explicitly adopt the support promise described in `docs/c-abi-downstream-baseline-decision.md`
- until that happens, the example improves confidence but does not by itself justify a stability declaration

## Recommended Decision Today

Recommendation: wait before declaring the current C ABI stable.

The current downstream-baseline policy recommendation is captured in `docs/c-abi-downstream-baseline-decision.md`.

Why this is the recommended call:

- the technical validation path is now much stronger than before
- the remaining uncertainty is no longer about whether the ABI can be consumed, but about whether maintainers want to make an explicit support promise around that consumption path
- that policy question is now tracked separately in `docs/c-abi-downstream-baseline-decision.md`
- that promise should be made intentionally, not inferred accidentally from tests and examples

A reasonable near-term decision rule is:

- if maintainers are willing to bless `examples/c_api/install_consume/` as the supported downstream baseline and also state that no near-term ABI break is expected, then the repository is close to a defensible stability declaration
- if maintainers are not yet willing to make that support promise, keep the ABI in draft status and preserve the current v1 surface unchanged
## Decision Summary

If maintainers had to decide today, the correct call is:

- do not declare the current C ABI stable yet
- keep the existing v1 surface unchanged while gathering missing evidence
- treat downstream-consumer credibility as the main blocker
- treat explicit no-break confirmation as the final governance check before any stability claim

## Smallest Remaining Path To Stability

The smallest credible path from the current state to a stability declaration is:

1. keep the current `rudp_c.h` surface unchanged
2. either bless `examples/c_api/install_consume/` as an explicitly supported downstream baseline, or add another credible consumer story that uses the existing ABI as-is
3. confirm in maintain/release docs that no near-term ABI break is expected
4. make the stability declaration in a release only after those points are true

## Non-Goals Of This Assessment

This page does not recommend:

- expanding the C ABI just to justify stability
- starting Rust work before the gate is satisfied
- broadening PlatformIO scope as part of the ABI decision
- waiting for a large ecosystem before ever making a stability call
