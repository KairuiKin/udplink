# Changelog

All notable changes to this project should be documented in this file.

## Unreleased

### Added

- N/A

### Changed

- N/A

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

