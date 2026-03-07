# Changelog

All notable changes to this project should be documented in this file.

## Unreleased

### Added

- Reliable UDP core for MCU-friendly environments (fixed queues, no runtime heap in core path).
- Handshake, heartbeat, timeout, SACK, adaptive retransmission.
- Session isolation, anti-replay, SipHash authentication.
- Manual and scheduled key rotation with acknowledgement convergence.
- Optional critical-section hooks for concurrency control.
- Self-test and benchmark executables.

### Changed

- Receive path now validates authentication before key-rotation policy checks.
- ACK bitmap build optimized from nested search to single queue scan.

