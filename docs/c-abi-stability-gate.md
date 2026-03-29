# C ABI Stability Gate

## Purpose

This page defines the minimum conditions required before the repository may call the current C ABI baseline "stable".

For the repository-wide engineering read of the current state, see `docs/c-abi-stability-assessment.md`.

It does not declare the ABI stable by itself.
It exists to stop the project from drifting between two bad states:

- freezing the ABI too early
- leaving the ABI in permanent draft mode with no decision point

## Current Position

Current repository position:

- the minimal C ABI exists in `include/rudp/rudp_c.h`
- the wrapper exists in `src/rudp_c.cpp`
- smoke coverage exists in `tests/c_api_test.cpp`
- installed-package C consumption exists in `tests/install_consume_c/`
- compatibility rules exist in `docs/c-abi-compatibility.md`

Current decision:

- the C ABI is implemented
- the C ABI is not yet declared long-term stable
- Rust or other FFI expansion should still wait until this gate is met and explicitly accepted

## Stability Gate

The first stable C ABI baseline may be declared only when all items below are true.

### 1. Surface discipline is still intact

- the public ABI remains intentionally small
- no manager-layer ABI has been added just to anticipate future bindings
- ownership and threading rules are still expressible in simple C terms

### 2. ABI-sensitive validation passes consistently

At minimum, the following must pass on the release candidate that would make the stability claim:

- `rudp_c_api_test`
- installed-package C consumer in `tests/install_consume_c/`
- full `python scripts/release_check.py`
- the normal C++ validation path remains green alongside the C path

### 3. Install and packaging shape are no longer in flux

- `include/rudp/rudp_c.h` is installed by default in the supported package path
- installed-package consumption does not require private headers or ad hoc include hacks
- exported package metadata and install layout are considered release-ready, not experimental

### 4. Documentation agrees on the contract

At minimum, these documents must describe the same ABI position:

- `docs/c-api-quickstart.md`
- `docs/c-abi-draft.md`
- `docs/c-abi-compatibility.md`
- this page
- `MAINTAIN.md`
- `docs/release-checklist.md`

### 5. There is at least one credible downstream use case

The project does not need a broad ecosystem before declaring the first stable ABI.
But it should have at least one concrete consumer story to justify freezing the baseline, for example:

- a real C consumer inside or outside the repository
- a Rust wrapper spike proposal that can stay within the current ABI
- another FFI consumer that exercises lifecycle, tick, send/receive, and metrics

The point is to freeze a baseline that serves an actual integration path, not an abstract maybe.

The technical assessment for this gate lives in `docs/c-abi-stability-assessment.md`, and the narrower policy decision about whether the in-repo C consumer is enough now lives in `docs/c-abi-downstream-baseline-decision.md`.

### 6. No known near-term breaking change is queued

Do not declare stability if maintainers already expect to break one of these soon after:

- function names or signatures in `rudp_c.h`
- field layout of existing `*_v1` structs
- ownership model or callback calling rules
- meaning of existing status codes
- meaning of `RUDP_ABI_VERSION`

If a near-term break is already likely, keep the ABI in draft status and resolve that design pressure first.

## What This Gate Does Not Require

The first stable ABI decision does not require all of the following:

- a large API surface
- manager-layer C bindings
- broad embedded-board certification
- a full Rust SDK
- a multi-platform packaging matrix across every host

Those may come later. They are not prerequisites for the first stability declaration.

## Initial Assessment Of Current Repo State

This is an engineering assessment of the repository as it exists today, not a formal stability declaration.

Current read:

- surface discipline: likely met
- ABI-sensitive tests and install-consume coverage: likely met, with installed-package C consumption now configured on both Linux and Windows CI paths
- install/package shape: likely close to ready
- docs alignment: met after the current documentation sync
- credible downstream consumer story: not clearly met yet beyond smoke-consumer coverage
- no near-term breaking change expected: should be confirmed explicitly before any stability claim

That means the repository looks closer to a decision point than before, but the safest current position is still:

- keep the ABI implemented but not yet declared stable
- treat real consumer evidence as the main remaining question before any stability declaration

## Declaration Rule

A release may say the C ABI is stable only if maintainers do all of the following together:

1. confirm every gate item above is met for that release candidate
2. update `docs/c-abi-compatibility.md` to reflect that the ABI is now an explicitly stable baseline
3. mention the stability declaration in release notes and changelog
4. treat future ABI-sensitive changes as additive-only unless a new ABI revision is announced

If those steps are not taken together, the repository should continue to describe the ABI as implemented but not yet declared stable.

## Default Decision Rule

If maintainers are unsure whether the gate is met, the default answer is:

- do not declare stability yet
- keep the existing v1 ABI unchanged
- gather the missing validation or consumer evidence first

## Practical Readiness Checklist

Before declaring the first stable ABI baseline, answer these questions with a clear yes:

- Is the current `rudp_c.h` surface still intentionally minimal?
- Do `rudp_c_api_test`, `tests/install_consume_c/`, and `scripts/release_check.py` pass on the candidate release?
- Are install/export rules settled enough that downstream users can consume the header cleanly?
- Do docs and maintain/release docs all describe the same ABI position?
- Is there at least one real consumer path that justifies freezing this baseline?
- Are maintainers not already planning a near-term ABI break?

If any answer is no, the ABI should remain in draft status.
