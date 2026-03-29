# C API Examples

This directory contains maintained C-facing examples that consume the public installed package instead of reaching into C++ headers directly.

## Current Example

- `install_consume/`: minimal installed-package C consumer example

This example is intentionally different from `tests/install_consume_c/`:

- `tests/install_consume_c/` is a smoke test owned by release validation
- `examples/c_api/install_consume/` is a readable reference example meant for downstream consumers

## What It Demonstrates

- including `rudp/rudp_c.h` from an installed package
- checking `rudp_abi_version()` and `rudp_version_string()`
- setting up `rudp_hooks_v1`
- creating, initializing, querying, and destroying `rudp_endpoint_handle`
- calling `rudp_endpoint_send()` and handling `RUDP_STATUS_NOT_CONNECTED`

## Build Against An Installed Package

After installing `udplink`, configure the example against the exported CMake package. On Unix-like systems the install prefix via `CMAKE_PREFIX_PATH` is sufficient. On Windows PowerShell, pointing `rudp_DIR` directly at `stage\lib\cmake\rudp` is more robust across generator/toolchain combinations.

### Unix-like

```bash
cmake -S examples/c_api/install_consume -B build-c-api-example -DCMAKE_PREFIX_PATH="$PWD/stage"
cmake --build build-c-api-example --config Release
./build-c-api-example/rudp_c_api_example
```

### Windows PowerShell

```powershell
cmake -S examples/c_api/install_consume -B build-c-api-example -Drudp_DIR:PATH="$PWD\stage\lib\cmake\rudp"
cmake --build build-c-api-example --config Release
.\build-c-api-example\Release\rudp_c_api_example.exe
```

## Scope

This example is intentionally small.
It proves clean C-level package consumption, but it does not by itself declare the ABI stable.
For the current stability decision, see:

- `docs/c-abi-stability-gate.md`
- `docs/c-abi-stability-assessment.md`
- `docs/c-abi-downstream-baseline-decision.md`