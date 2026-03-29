# Board Bring-Up Report Example

This page is a filled example of the `Board Bring-Up Report` issue template.

It is not a real validation record.
Its purpose is to show maintainers what a useful hardware-backed report should look like.

## Summary

Arduino Mega 2560 + W5100 + PlatformIO + `rudp_example_udp_peer`

## Validation Result

pass

## Environment

- Date: 2026-03-29
- Operator: maintainer-example
- Board: Arduino Mega 2560
- Ethernet shield / chipset: W5100-compatible shield
- PlatformIO environment: `megaatmega2560_w5100`
- Host OS: Windows 11
- Host peer command: `.\build\Release\rudp_example_udp_peer.exe 192.168.1.100 8889 192.168.1.177 8888`
- Board IP / port: `192.168.1.177:8888`
- Host IP / port: `192.168.1.100:8889`
- Local edits from repository defaults: none

## Bring-Up Checklist

- [x] board flashed successfully
- [x] serial monitor opened successfully
- [x] host peer started on the expected address and port
- [x] at least one side showed handshake progress
- [x] both sides reached connected state `2`
- [x] board-to-host app payload was observed
- [x] host-to-board app payload was observed

## Observed Board-Side Lines

```text
=== Arduino udplink example ===
Local IP: 192.168.1.177
[RUDP] state=1
[RUDP] state=2
[APP] sent sensor packet #1
[RUDP] recv len=11 host-ping-1
```

## Observed Host-Side Lines

```text
[peer] waiting on 192.168.1.100:8889 for board 192.168.1.177:8888
[peer] state=1
[peer] state=2
[peer] sent host-ping-1
[peer] delivered packet #1 len=9 data=sensor-01
```

## Failures Or Deviations

None for this example.

## Notes

This is the minimum level of detail that should appear in a real board-backed validation issue.
If a real run fails, the report should still capture exact commands, observed logs, and any local config changes.
