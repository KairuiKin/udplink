---
name: Board Bring-Up Report
about: Record a real board-backed validation run for a documented udplink path.
title: "board: bring-up report for <board + network path>"
labels: ["validation", "embedded"]
assignees: []
---

## Summary

Describe the exact path you ran.

Example:

- Arduino Mega 2560 + W5100 + PlatformIO + `rudp_example_udp_peer`

## Validation Result

Choose one:

- pass
- partial
- fail

## Environment

- Date:
- Operator:
- Board:
- Ethernet shield / chipset:
- PlatformIO environment:
- Host OS:
- Host peer command:
- Board IP / port:
- Host IP / port:
- Local edits from repository defaults:

## Bring-Up Checklist

- [ ] board flashed successfully
- [ ] serial monitor opened successfully
- [ ] host peer started on the expected address and port
- [ ] at least one side showed handshake progress
- [ ] both sides reached connected state `2`
- [ ] board-to-host app payload was observed
- [ ] host-to-board app payload was observed

## Observed Board-Side Lines

Paste the key serial lines that show startup, connect, and payload delivery.

```text
<paste here>
```

## Observed Host-Side Lines

Paste the key host peer lines that show startup, connect, and payload delivery.

```text
<paste here>
```

## Failures Or Deviations

Describe anything that did not match the documented path.

## Notes

Anything else a maintainer should know to reproduce or interpret the run.
