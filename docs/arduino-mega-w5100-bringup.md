# Reference Bring-Up: Arduino Mega 2560 + W5100

## Status

This page defines the first recommended board-backed bring-up path for `udplink`.

Current state:

- board target: Arduino Mega 2560
- network shield/chipset: W5100-class Ethernet
- build path: PlatformIO local build
- host peer: `examples/socket_peer.cpp`
- repository status: drafted and execution-ready
- hardware confirmation status: not yet maintainer-confirmed on a real board

This is a narrow validation path, not a broad board-support promise.

## Goal

The goal is to provide one reproducible path that is easier to follow than the generic template README:

1. build the Arduino sketch with PlatformIO
2. run one host-side `udplink` UDP peer on a PC
3. observe handshake and app-level traffic in both directions
4. record enough evidence to decide whether this path should become the repository's first proven board-backed route

## Scope Boundary

This page is intentionally narrow.

It is trying to answer only one question:

- can one real Arduino Mega 2560 + W5100 setup interoperate with the current desktop `udplink` build through the current Arduino template?

It is not trying to certify:

- every Arduino board
- every Ethernet shield or Ethernet library combination
- PlatformIO as a CI-backed support matrix
- long-duration soak behavior across every LAN condition

## Repository Pieces

Board side:

- `examples/arduino/arduino_udp/arduino_udp.ino`
- `examples/arduino/arduino_udp/platformio.ini`

Host side:

- `examples/socket_peer.cpp`
- target name: `rudp_example_udp_peer`

Related references:

- `examples/arduino/arduino_udp/README.md`
- `docs/project-status.md`
- `docs/arduino-mega-w5100-maintainer-run.md`
- `docs/issue-drafts-post-v0.1.2.md`

## Network Topology

Default board-side settings in `arduino_udp.ino`:

- board IP: `192.168.1.177`
- host peer IP: `192.168.1.100`
- board local UDP port: `8888`
- host local UDP port: `8889`

That means:

- the Arduino board binds `8888` and sends to `192.168.1.100:8889`
- the host peer should bind `192.168.1.100:8889` and send to `192.168.1.177:8888`

If your LAN uses different addresses, update the constants in `arduino_udp.ino` and keep the host command in sync.

## Preconditions

Hardware:

- Arduino Mega 2560
- W5100-compatible Ethernet shield
- USB cable for flashing and serial monitor
- Ethernet connection on the same LAN as the host PC

Software:

- PlatformIO available locally
- CMake toolchain for building the host example
- a host machine whose IPv4 address matches `kPeerIp`

Operator checks before starting:

- verify the board IP is not already used by another device on the LAN
- verify local firewall rules allow UDP on the selected host port
- verify the board and host use the same auth keys and payload assumptions

## Build The Host Peer

### Windows PowerShell

Raw command path:

```powershell
cmake -S . -B build -DRUDP_BUILD_EXAMPLES=ON -DRUDP_BUILD_TESTS=ON
cmake --build build --config Release --target rudp_example_udp_peer
.\build\Release\rudp_example_udp_peer.exe 192.168.1.100 8889 192.168.1.177 8888
```

Maintainer fast path:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\prepare_mega_w5100_run.ps1
.\logs\board-runs\<run-id>\validate-run.ps1
.\logs\board-runs\<run-id>\start-host-peer.ps1
# after a real run:
.\logs\board-runs\<run-id>\finalize-report.ps1
.\logs\board-runs\<run-id>\file-issue.ps1
.\logs\board-runs\<run-id>\bundle-artifacts.ps1
```

### Linux / macOS

```bash
cmake -S . -B build -DRUDP_BUILD_EXAMPLES=ON -DRUDP_BUILD_TESTS=ON
cmake --build build --target rudp_example_udp_peer
./build/rudp_example_udp_peer 192.168.1.100 8889 192.168.1.177 8888
```

Expected early output:

```text
[peer] waiting on 192.168.1.100:8889 for board 192.168.1.177:8888
```

Later, once the board starts and handshake succeeds, you should see a state transition to connected and periodic host sends.

## Build And Flash The Board

From the repository root:

```bash
cd examples/arduino/arduino_udp
pio run -e megaatmega2560_w5100
pio run -e megaatmega2560_w5100 -t upload
pio device monitor -b 115200
```

If your serial port is not auto-detected, pass the correct upload or monitor port through PlatformIO settings.

## Maintainer Execution Checklist

Run this as a real checklist, not just as reading material.

### Preparation

- [ ] confirm the board, shield, LAN, and host machine match the intended topology
- [ ] confirm `arduino_udp.ino` addresses and ports match the current LAN
- [ ] build `rudp_example_udp_peer`
- [ ] start the host peer before resetting or powering the board
- [ ] open a serial monitor at `115200`

### Bring-Up Run

- [ ] flash the board successfully
- [ ] observe board startup banner and local IP print
- [ ] observe host peer waiting on the expected address and port
- [ ] observe handshake progression to `state=1` on at least one side
- [ ] observe connected state `state=2` on both board and host
- [ ] observe at least one board-to-host delivered payload
- [ ] observe at least one host-to-board delivered payload

### Capture Before Declaring Success

- [ ] record the exact board model and Ethernet shield used
- [ ] record the PlatformIO environment name
- [ ] record the host OS and command used to launch `rudp_example_udp_peer`
- [ ] save or paste the key serial-monitor lines showing connect and delivery
- [ ] save or paste the key host-peer lines showing connect and delivery
- [ ] note any local edits made to IPs, ports, or auth settings

## Expected Bring-Up Sequence

1. Start the host peer first.
2. Power or reset the Arduino board.
3. Watch the serial monitor for Ethernet initialization and state transitions.
4. Watch the host peer for `state=2` and delivered packets.

Expected board-side signals:

- `=== Arduino udplink example ===`
- `Local IP: ...`
- `[RUDP] state=1` during handshake progress
- `[RUDP] state=2` once connected
- `[APP] sent sensor packet #...` every 30 seconds after connection
- `[RUDP] recv ...` when host peer app packets arrive

Expected host-side signals:

- `[peer] state=1` then `[peer] state=2`
- `[peer] sent host-ping-1`
- `[peer] delivered packet #... len=9 data=...` for board sensor payloads

## Acceptance Criteria

Treat this path as maintainer-confirmed only if all of the following are true in one real run:

- the host peer starts and binds the expected local address and port
- the board prints its local IP and enters handshake progress
- both sides reach connected state `2`
- app-level payload delivery is observed in both directions
- the operator captured enough evidence that another maintainer could review the run without guesswork

If any of those conditions are missing, the path should stay in drafted status even if some packets moved.

## No-Device Readiness Check

Even without hardware connected yet, you can still prove that the repository-side run pack is coherent:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\prepare_mega_w5100_run.ps1
.\logs\board-runs\<run-id>\validate-run.ps1
```

That check does not prove board success. It only proves that the run folder, helper scripts, and summary/report templates are ready for a real maintainer run.

## Evidence Record Template

Copy this block into an issue, PR comment, or maintainer note when a real run is performed.
If you used the Windows maintainer fast path, `.\logs\board-runs\<run-id>\file-issue.ps1` can open the repository-native issue for you after the report is finalized.
If you need a single handoff archive, `.\logs\board-runs\<run-id>\bundle-artifacts.ps1` packs the evidence, issue preview, and helper scripts into one zip file.
Otherwise, open a GitHub issue with the `Board Bring-Up Report` template in `.github/ISSUE_TEMPLATE/board-bringup-report.md`.
See `docs/board-bringup-report-example.md` for a filled example of the expected report shape.

```text
Board validation result: pass|partial|fail
Date:
Operator:
Board:
Ethernet shield:
PlatformIO environment:
Host OS:
Host peer command:
Board IP / port:
Host IP / port:
Local edits from defaults:
Observed board-side connect lines:
Observed host-side connect lines:
Observed board-to-host payload line:
Observed host-to-board payload line:
Notes / failures:
```

## Fast Failure Checks

If the board never reaches connected:

- verify the host machine really owns the IP configured as `kPeerIp`
- verify the board IP does not conflict with another device
- verify both sides use the same UDP ports
- verify both sides use the same auth keys
- verify the Ethernet shield is actually initialized and linked

If the host sees no packets at all:

- confirm the board and host are on the same IPv4 subnet
- confirm local firewall rules allow UDP on port `8889`
- confirm the board-side `g_udp.begin(kLocalPort)` succeeds

If handshake succeeds but app traffic is missing:

- confirm the host peer stays running after connect
- confirm the board serial log shows `[APP] sent sensor packet #...`
- confirm `max_payload` remains `96` on both sides

## What This Path Proves

This path is intended to prove one practical thing:

- a real board can interoperate with a host-side `udplink` endpoint using the current Arduino template and current desktop library build

It does not prove:

- every Arduino board works
- every Ethernet shield/library combination works
- PlatformIO is a release-gated support matrix

## Roadmap Meaning

If maintainers later run and confirm this exact path on hardware with the evidence template above, it can become the repository's first explicitly proven board-backed path.

Until then, it remains the best-defined execution-ready route for local validation and issue reproduction.
