# Changelog

All notable changes to this project should be documented in this file.

## Unreleased

### Added

- Minimal C ABI surface via `include/rudp/rudp_c.h` and `src/rudp_c.cpp`.
- `tests/c_api_test.cpp` smoke coverage for C ABI lifecycle, send, query, and failure-path checks.
- `tests/install_consume_c` installed-package consumer smoke for the public C header.
- Docs pages for C API quickstart, C ABI draft, and C ABI compatibility policy.
- Arduino-first PlatformIO local skeleton under `examples/arduino/arduino_udp/`.

### Changed

- `scripts/release_check.py` now validates both C++ and C install-consume paths.
- CI now runs `rudp_c_api_test` and installed-package C consumer validation.
- README and docs navigation now expose C API and ABI policy entry points directly.
- Project version metadata now matches released `0.1.2` artifacts.

### Fixed

- C ABI release/readiness checks are now included in the release checklist instead of being implicit.

## 0.1.2 - 2026-03-28

### Added

- Deterministic `rudp_reliability_test` for scripted loss/reorder regression coverage.
- Standalone `tests/install_consume` package-consumer smoke project.
- `scripts/release_check.py` for one-command local release verification.
- Docs pages for benchmarks, API mapping, release plan, release checklist, PR draft, and GitHub release draft.

### Changed

- Embedded example templates updated to the current callback-based API.
- README and contribution docs now distinguish smoke coverage from reliability regression coverage.
- CI now validates install/export output with a standalone `find_package(rudp)` consumer smoke build.
- Benchmark smoke is now executed in CI, and benchmark methodology/sample output is documented.
- Added an API mapping page to keep embedded/example integrations aligned with the current public interface.

### Fixed

- `Endpoint` re-initialization now resets protocol state explicitly before applying a new config.
- `ConnectionManager` initialization no longer relies on raw object-level zeroing for endpoint slots.

## 0.1.1 - 2026-03-27

### Added

- Reliable UDP core for MCU-friendly environments (fixed queues, no runtime heap in core path).
- Handshake, heartbeat, timeout, SACK, adaptive retransmission.
- Session isolation, anti-replay, SipHash authentication.
- Manual and scheduled key rotation with acknowledgement convergence.
- Optional critical-section hooks for concurrency control.
- Self-test and benchmark executables.
- Safer `ConnectionManager` query APIs: `IsConnected`, `GetConnectionState`, `GetStats`, `GetRuntimeMetrics`.
- Docs snippet sync script to keep docs examples aligned with source: `scripts/sync_docs_snippets.py`.

### Changed

- `ConnectionManager` locking and slot lifecycle hardened for concurrent usage.
- Manager test and manager example no longer rely on large stack allocation for manager instances.
- Receive path now validates replay window before ACK handling.
- Documentation updated to reflect current APIs and remove stale links.
