# Post-v0.1.2 Backlog

This file separates follow-ups that are already done from work that is still open after the current post-`v0.1.2` push.

## Completed follow-ups

### 1. Expand board-specific embedded adaptation notes

Status: done.

Delivered via:

- `examples/embedded/README.md`
- `examples/stm32/README.md`
- `examples/esp32/README.md`
- `examples/arduino/arduino_udp/README.md`
- `examples/raspberry_pi/README.md`

### 2. Add minimal automated embedded compile validation

Status: done.

Delivered via:

- `tests/embedded_compile/`
- CI compile-only coverage for the selected host-side embedded template paths

### 3. Improve docs site navigation and content density

Status: done for the current documentation baseline.

Delivered via:

- `docs/index.html`
- `docs/_sidebar.md`
- `docs/api-mapping.md`
- `docs/release-checklist.md`
- README/docs cross-links for build, test, examples, install, and C API entry points

### 4. Archive benchmark samples across releases

Status: done.

Delivered via:

- `docs/benchmarks.md`

### 5. Track `ConnectionManager::Find()` as compatibility-only API

Status: done.

Delivered via:

- `include/rudp/manager.hpp`
- `README.md`
- `README.en.md`
- `tests/manager_test.cpp`

### 6. Evaluate PlatformIO and Rust binding scope

Status: done.

Delivered via:

- `docs/platformio-rust-scope.md`
- `examples/arduino/arduino_udp/`

Decision:

- keep PlatformIO support narrow and Arduino-first
- keep Rust blocked until the C ABI baseline is declared stable

### 7. Define a minimal C ABI before Rust binding work

Status: done.

Delivered via:

- `include/rudp/rudp_c.h`
- `src/rudp_c.cpp`
- `tests/c_api_test.cpp`
- `tests/install_consume_c/`
- `docs/c-api-quickstart.md`
- `docs/c-abi-draft.md`

### 8. Formalize the C ABI compatibility contract

Status: done.

Delivered via:

- `docs/c-abi-compatibility.md`
- `docs/release-checklist.md`
- `scripts/release_check.py`
- CI coverage for `rudp_c_api_test` and installed-package C consumption

### 9. Define the first C ABI stability gate

Status: done.

Delivered via:

- `docs/c-abi-stability-gate.md`
- `docs/c-abi-compatibility.md`
- `docs/release-checklist.md`
- `MAINTAIN.md`

### 10. Add multi-platform C consumer validation

Status: done.

Delivered via:

- `.github/workflows/ci.yml` Linux installed-package C consumer validation
- `.github/workflows/ci.yml` Windows installed-package C consumer validation
- installed-package C++ and C consumers now run on both paths

## Open backlog

### 1. Decide whether to declare the first stable C ABI baseline

**Title**

`api: decide whether the current C ABI baseline should be declared stable`

**Body**

### Summary

The repository now has a written ABI stability gate, a current-state assessment, multi-platform install-consume coverage, and a maintained in-repo C consumer example that is executed by CI and `scripts/release_check.py`. The next step is to decide whether that is enough to close the downstream-consumer requirement for a release claim.

### Why

- the project now has enough structure to make a real stability decision
- remaining ambiguity should come from concrete gaps, not from missing process
- Rust or other FFI work should stay blocked until this decision is made explicitly

### Scope

- review the current C ABI against `docs/c-abi-stability-gate.md`, `docs/c-abi-stability-assessment.md`, and `docs/c-abi-downstream-baseline-decision.md`
- identify whether maintainers are willing to bless `examples/c_api/install_consume/` as the supported downstream baseline, and whether an explicit no-break confirmation is ready
- default to wait if either answer is still unclear
- either declare the baseline stable in a release, or document exactly why it stays draft

### Acceptance Criteria

- maintain docs state clearly whether the gate is currently met
- if the gate is not met, the remaining decision or missing conditions are named explicitly
- if the gate is met, release docs and changelog make the stability claim intentionally
- if the gate is not yet met, the repository keeps the current v1 ABI unchanged without making a stability promise

### 2. Add one reproducible board-backed bring-up path

**Title**

`examples: provide one end-to-end board-backed validation path for a reference target`

**Body**

### Summary

Compile-only checks help prevent API drift, but they do not prove that any embedded path is operational on real hardware. The repository now has a drafted reference path in `docs/arduino-mega-w5100-bringup.md`; the next step is to confirm it on real hardware.

### Why

- embedded users still face the last-mile risk of transport, timer, and peer wiring mistakes
- one well-documented board path is more valuable than many unverified templates
- it would turn the examples section from `reference only` toward `reference plus one proven path`

### Scope

- use `docs/arduino-mega-w5100-bringup.md` as the first candidate path unless another board has stronger maintainer ownership
- document exact build/run prerequisites, transport assumptions, validation steps, and evidence capture expectations
- keep the scope to one reproducible path rather than trying to certify every board family

### Acceptance Criteria

- one board path has a maintainer-confirmed start-to-finish bring-up document
- the document includes expected packet flow or observable success criteria
- the document includes a lightweight evidence template that maintainers can attach to a real run
- maintain docs clearly distinguish this proven path from template-only examples

### 3. Start a Rust wrapper spike only after the ABI gate is met

**Title**

`ffi: prototype a Rust wrapper strictly on top of rudp_c.h after the ABI stability gate is met`

**Body**

### Summary

Once the project declares the initial C ABI baseline stable, a small Rust wrapper spike becomes a reasonable next ecosystem experiment.

### Why

- Rust reach is useful, but not if it pulls ABI churn into two surfaces at once
- forcing the wrapper through `rudp_c.h` tests whether the C ABI is actually sufficient
- a small spike is enough to identify missing ergonomics before committing to long-term support

### Scope

- do not begin before the ABI stability gate is satisfied
- bind only the currently supported C ABI surface
- keep the first wrapper minimal: lifecycle, tick, send, receive, and metrics

### Acceptance Criteria

- the wrapper builds only against the public C header
- no direct dependency on C++ classes leaks into the Rust integration
- any missing ABI pieces are fed back as focused C ABI extensions, not ad hoc wrapper hacks
