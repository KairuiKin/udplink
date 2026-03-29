# Maintainer Run Pack: Arduino Mega 2560 + W5100

Snapshot date: 2026-03-29.

This page is the operator-focused companion to `docs/arduino-mega-w5100-bringup.md`.

Use it when a maintainer wants to do one real board-backed run and leave behind enough evidence for another maintainer to review without guessing.

## Goal

Finish one complete maintainer run that produces all of the following:

- host-peer log file
- board-side serial notes or captured serial output
- one filed or ready-to-file `Board Bring-Up Report`
- a clear pass / partial / fail outcome

## Related Files

- `docs/arduino-mega-w5100-bringup.md`
- `docs/board-bringup-report-example.md`
- `.github/ISSUE_TEMPLATE/board-bringup-report.md`
- `scripts/run_peer_capture.py`
- `scripts/init_board_run.py`
- `scripts/prepare_mega_w5100_run.ps1`

## Suggested Working Folder

Preferred path: initialize a run folder with the helper script so the summary and notes templates already exist.

### Windows PowerShell

Fast path that prepares the run directory, rebuilds `rudp_example_udp_peer`, scans serial ports, prints the exact host-peer / upload / monitor commands, and writes ready-to-run PowerShell helper scripts into the run directory:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\prepare_mega_w5100_run.ps1
```

Fallback if you only want the run directory templates:

```powershell
python scripts/init_board_run.py --board "Arduino Mega 2560" --shield "W5100-compatible shield"
```

### Linux / macOS

```bash
python scripts/init_board_run.py --board "Arduino Mega 2560" --shield "W5100-compatible shield"
```

This prints the created run directory and initializes:

- `run-summary.txt`
- `board-serial-notes.txt`
- `board-bringup-report-draft.md`

On Windows, `prepare_mega_w5100_run.ps1` also writes:

- `start-host-peer.ps1`
- `build-board.ps1`
- `upload-board.ps1`
- `monitor-board.ps1`

Fallback manual directory creation is still fine if you do not want to use the helper.

## Step 1: Build The Host Peer

### Windows PowerShell

```powershell
cmake -S . -B build -DRUDP_BUILD_EXAMPLES=ON -DRUDP_BUILD_TESTS=ON
cmake --build build --config Release --target rudp_example_udp_peer
```

### Linux / macOS

```bash
cmake -S . -B build -DRUDP_BUILD_EXAMPLES=ON -DRUDP_BUILD_TESTS=ON
cmake --build build --target rudp_example_udp_peer
```

## Step 2: Start The Host Peer With Log Capture

### Windows PowerShell

If you used `prepare_mega_w5100_run.ps1`, prefer running the generated helper directly:

```powershell
.\logs\board-runs\<run-id>\start-host-peer.ps1
```

Equivalent raw command:

```powershell
python scripts/run_peer_capture.py --log "$runDir/host-peer.log" -- .\build\Release\rudp_example_udp_peer.exe 192.168.1.100 8889 192.168.1.177 8888
```

### Linux / macOS

```bash
python scripts/run_peer_capture.py --log "$run_dir/host-peer.log" -- ./build/rudp_example_udp_peer 192.168.1.100 8889 192.168.1.177 8888
```

Expected early output:

```text
[peer] waiting on 192.168.1.100:8889 for board 192.168.1.177:8888
```

Keep this process running during the board bring-up.

## Step 3: Build And Flash The Board

```bash
cd examples/arduino/arduino_udp
pio run -e megaatmega2560_w5100
pio run -e megaatmega2560_w5100 -t upload
pio device monitor -b 115200
```

If you used the Windows fast path, the same commands are already written into the generated `build-board.ps1`, `upload-board.ps1`, and `monitor-board.ps1` files inside the run directory.

If the serial monitor tool can save output directly in your local setup, save it into the same run folder.
If it cannot, still paste the important serial lines into a text file such as `board-serial-notes.txt`.

## Step 4: Capture The Minimum Evidence

Before you close the run, make sure the run folder contains at least:

- `host-peer.log`
- either a saved serial log or a manual notes file with the key board-side lines
- a short text note recording board model, shield, host OS, IPs, ports, and local edits

Suggested note file names:

- `board-serial-notes.txt`
- `run-summary.txt`

## Step 5: Decide The Outcome

Use this rule:

- `pass`: both sides reach connected state `2`, and app payloads are observed in both directions
- `partial`: some transport activity occurs, but connect or app delivery is incomplete
- `fail`: the path does not reach a meaningful handshake or there is not enough evidence to review it

## Step 6: File Or Prepare The Report

Use one of these:

- preferred: open a GitHub issue with `.github/ISSUE_TEMPLATE/board-bringup-report.md`
- fallback: fill a local draft using `docs/board-bringup-report-example.md` as the shape reference

When filing the report, attach or paste:

- the exact host peer command
- the key board-side lines
- the key host-side lines
- the final result: pass / partial / fail
- any local edits from repository defaults

## Review Checklist

A maintainer reviewing the run should be able to answer all of the following from the report without asking for another round of context:

- what exact board path was used?
- what exact host command was used?
- did both sides reach connected state `2`?
- was app payload delivery observed in both directions?
- were there any local deviations from the documented defaults?

## If The Run Fails

Do not discard the evidence.

A useful failing run still includes:

- the exact host peer command
- the board-side lines up to the point of failure
- the host-side lines up to the point of failure
- the operator's guess about where the path diverged from the documented route

That is enough to make the next maintainer attempt cheaper.
