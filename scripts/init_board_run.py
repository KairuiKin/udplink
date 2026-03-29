#!/usr/bin/env python3
"""Initialize a board-run workspace with summary and notes templates.

Example:
    python scripts/init_board_run.py --board "Arduino Mega 2560" --shield "W5100-compatible shield"
"""

from __future__ import annotations

import argparse
from datetime import datetime
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
RUNS_ROOT = ROOT / "logs" / "board-runs"

RUN_SUMMARY_TEMPLATE = """Board path: Arduino Mega 2560 + W5100 + PlatformIO + rudp_example_udp_peer
Run result: pass|partial|fail
Date:
Operator:
Board:
Ethernet shield / chipset:
PlatformIO environment:
Host OS:
Host peer command:
Board IP / port:
Host IP / port:
Local edits from defaults:
Notes:
"""

SERIAL_NOTES_TEMPLATE = """Board-side serial notes
=======================

Paste or summarize the key serial lines here:

- startup banner:
- local IP:
- handshake progress:
- connected state:
- app payload send:
- app payload receive:
- failures or deviations:
"""

REPORT_DRAFT_TEMPLATE = """## Summary

Arduino Mega 2560 + W5100 + PlatformIO + `rudp_example_udp_peer`

## Validation Result

pass|partial|fail

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

```text
<paste here>
```

## Observed Host-Side Lines

```text
<paste here>
```

## Failures Or Deviations

Describe anything that did not match the documented path.

## Notes

Anything else a maintainer should know to reproduce or interpret the run.
"""


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--run-id", help="Optional explicit run id; default is current timestamp")
    parser.add_argument("--board", default="Arduino Mega 2560")
    parser.add_argument("--shield", default="W5100-compatible shield")
    parser.add_argument("--platformio-env", default="megaatmega2560_w5100")
    return parser.parse_args()


def write_text(path: Path, text: str) -> None:
    path.write_text(text, encoding="utf-8", newline="\n")


def main() -> int:
    args = parse_args()
    run_id = args.run_id or datetime.now().strftime("%Y%m%d-%H%M%S")
    run_dir = RUNS_ROOT / run_id
    run_dir.mkdir(parents=True, exist_ok=True)

    summary = RUN_SUMMARY_TEMPLATE.replace("Date:", f"Date: {datetime.now().date().isoformat()}")
    summary = summary.replace("Board:\n", f"Board: {args.board}\n")
    summary = summary.replace("Ethernet shield / chipset:\n", f"Ethernet shield / chipset: {args.shield}\n")
    summary = summary.replace("PlatformIO environment:\n", f"PlatformIO environment: {args.platformio_env}\n")

    write_text(run_dir / "run-summary.txt", summary)
    write_text(run_dir / "board-serial-notes.txt", SERIAL_NOTES_TEMPLATE)
    write_text(run_dir / "board-bringup-report-draft.md", REPORT_DRAFT_TEMPLATE)

    print(run_dir)
    print(run_dir / "run-summary.txt")
    print(run_dir / "board-serial-notes.txt")
    print(run_dir / "board-bringup-report-draft.md")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
