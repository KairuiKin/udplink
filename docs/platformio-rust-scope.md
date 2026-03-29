# PlatformIO and Rust Scope

## Purpose

This note turns two vague ecosystem ideas into explicit roadmap decisions:

- PlatformIO support: narrow, Arduino-first, template-level support
- Rust bindings: defer until there is a stable C ABI layer to bind against

The goal is to grow adoption without creating a support surface the project cannot maintain.

## Current Constraints

### PlatformIO

- The repository already has an Arduino Ethernet reference template.
- The main CI pipeline does not provision Arduino or PlatformIO toolchains.
- The template still needs board-specific memory and network tuning.
- The current public API is C++11-first and is documented around direct C++ integration.

### Rust

- There is now an initial C ABI header/wrapper/smoke-test skeleton, but it is not yet declared stable.
- The public API exposes C++ classes, callbacks, and ownership patterns directly.
- The manager and endpoint APIs are still evolving at the source level.
- Any binding would need release discipline, ABI policy, and unsafe usage documentation.

## Decision Summary

### PlatformIO

Decision: support a minimal Arduino-first story, not a full multi-board matrix.

What "supported" means in this project:

- the Arduino example README explains a PlatformIO-based local bring-up path
- the repository includes a minimal local skeleton in `examples/arduino/arduino_udp/platformio.ini`
- one reference configuration is documented around Ethernet-capable Arduino boards in `docs/arduino-mega-w5100-bringup.md`
- the repository does not promise CI coverage for PlatformIO toolchains yet
- board runtime validation remains a local responsibility

What is explicitly out of scope for now:

- broad board matrix maintenance
- PlatformIO CI with external platform downloads
- guaranteeing compatibility across every Ethernet shield and library version
- non-Arduino PlatformIO environments as a first milestone

Recommended first milestone:

1. keep `docs/arduino-mega-w5100-bringup.md` as the first reference board path for local validation
2. keep udplink sources consumed directly from the repository
3. treat failures outside that path as community-contributed follow-ups, not core release blockers

Validation strategy:

- documentation review for path completeness
- local smoke build by a maintainer when touching the Arduino template
- no release gate on PlatformIO until the workflow becomes reproducible in CI

Maintenance cost:

- low to medium if kept to one documented board profile
- high if expanded into a board/library compatibility matrix

### Rust

Decision: defer Rust bindings until a small stable C ABI exists.

Why defer now:

- binding the current C++ surface directly would lock the project into fragile FFI choices
- callback and lifetime rules are easier to stabilize in C than in cross-language C++ FFI
- testing, packaging, and versioning burden would exceed the current maintainer capacity

Prerequisites before any Rust binding work starts:

1. define a minimal C-facing API for endpoint lifecycle, send, receive, tick, and stats
2. document ownership, threading, and callback rules in ABI-safe terms
3. add C-level tests that cover the wrapper independently from C++ examples
4. decide versioning and symbol-compatibility policy for the C layer
5. only then design a thin safe Rust wrapper around that ABI
6. use `docs/c-abi-draft.md` as the starting point for the first wrapper design review

What a future minimal Rust surface should expose:

- endpoint init and shutdown
- packet ingress and egress callbacks
- periodic tick driving
- connection state query
- runtime metrics and stats export

What should stay out of the first Rust milestone:

- manager abstraction if the C ABI has not proven stable yet
- async runtime integrations
- zero-copy promises across the FFI boundary
- multiple crate targets or no_std support claims

Validation strategy:

- C ABI tests in the core repository
- one Rust smoke example around the stable C wrapper
- no public crate release before ABI rules are written down

Maintenance cost:

- medium for a tiny C ABI
- high for a public Rust crate with compatibility expectations

## Roadmap Position

### v0.2.x

- finish docs and example usability work already in flight
- keep PlatformIO as a documented Arduino bring-up option only
- keep Rust binding implementation blocked until the C ABI stops moving

### Later, after C ABI exists

- reassess whether a Rust crate improves adoption enough to justify support burden
- only proceed if the C ABI is already tested and versioned

## Practical Recommendation

If maintainer time is limited, the correct order is:

1. keep improving C++ docs, examples, and release verification
2. make Arduino/PlatformIO onboarding understandable with one narrow path
3. stabilize a minimal C ABI only when there is concrete downstream demand
4. build Rust on top of that ABI, not on the current C++ headers
