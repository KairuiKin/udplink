# C ABI Compatibility Policy

## Purpose

This page defines what the project means by "compatible" for the current C ABI.

It exists to stop the ABI from drifting implicitly while the project is still deciding whether to build higher-level bindings such as Rust.

## Current Status

The C ABI is implemented, but it is not yet declared long-term stable across arbitrary future releases.

The decision gate for that declaration is defined in `docs/c-abi-stability-gate.md`.

Current repository baseline:

- public header: `include/rudp/rudp_c.h`
- wrapper implementation: `src/rudp_c.cpp`
- smoke validation: `tests/c_api_test.cpp`
- installed-package C consumer: `tests/install_consume_c/`
- checked-in v1 surface snapshot: `docs/c-abi-surface-v1.json`

## Compatibility Scope

For the current line, the project aims to preserve source and binary compatibility for the exported C symbols that already exist in `rudp_c.h`, unless a future release explicitly documents a breaking ABI revision.

The repository now keeps a checked-in snapshot of that public surface in `docs/c-abi-surface-v1.json`, and `python scripts/check_c_abi_surface.py` fails if `include/rudp/rudp_c.h` drifts away from that baseline without an intentional snapshot update.

That means maintainers should treat these items as compatibility-sensitive:

- exported function names beginning with `rudp_`
- opaque handle ownership model
- meaning of existing `rudp_status` values
- field order and field types of existing `*_v1` structs
- meaning of `RUDP_ABI_VERSION`

## What May Change Without Breaking ABI

The following changes are allowed within the same ABI version:

- internal wrapper implementation details in `src/rudp_c.cpp`
- internal C++ object layout behind `rudp_endpoint_handle`
- stricter internal validation that still returns existing status codes
- documentation wording and examples
- new functions or new `*_v2` structs added alongside existing v1 entries

## What Must Not Change In ABI v1

Do not make these changes under the current ABI version:

- renaming or removing existing exported `rudp_` functions
- reordering, resizing, or retagging fields in `rudp_config_v1`, `rudp_stats_v1`, or `rudp_runtime_metrics_v1`
- changing the numeric meaning of existing `rudp_status` codes
- changing the meaning of `RUDP_ABI_VERSION` without updating policy and tests
- exposing internal pointers or requiring foreign code to understand C++ object layout

## Versioning Rules

### Additive Changes

If the project needs more API surface without breaking callers, prefer additive growth:

- add new functions
- add new enum values only when old callers remain valid
- introduce new structs with explicit suffixes such as `rudp_config_v2`

### Breaking Changes

If the project must break ABI, it should do all of the following together:

1. bump `RUDP_ABI_VERSION`
2. document the break in release notes and migration docs
3. keep the old entry points only if there is a deliberate compatibility bridge
4. update tests to validate the new contract explicitly

## Threading Contract

The current ABI contract is single-owner per handle.

Callers must not assume thread safety for concurrent operations on the same handle.
If a binding or host application needs multi-threaded access, it must serialize access externally.

Changing this rule later would be a semantic compatibility event and must not happen silently.

## Validation Requirements Before Release

Any release that touches `rudp_c.h`, `src/rudp_c.cpp`, install rules, or C ABI docs should pass at least:

1. `python scripts/check_c_abi_surface.py`
2. `rudp_c_api_test`
3. installed-package C consumer build/run from `tests/install_consume_c/`
4. full `scripts/release_check.py`

If those checks are not run, the release should not claim ABI-sensitive changes are verified.

## Guidance For Future Rust Work

Rust work should build on this policy, not bypass it.

That means:

- no direct Rust-to-C++ binding against `rudp.hpp`
- only wrap the C ABI that is covered by this policy and tests
- do not treat the ABI as stable beyond what this document explicitly promises

## Practical Rule

If a maintainer is unsure whether a change is ABI-safe, the default answer should be:

- keep the existing v1 entry points unchanged
- add a new symbol or new suffixed struct instead
- update tests and docs before expanding binding work
