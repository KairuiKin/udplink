# C API Quickstart

This page shows the narrow, currently-supported `udplink` C entry points.

## Scope

Use the C API when you need:

- a language-neutral endpoint wrapper
- an opaque handle instead of direct C++ class ownership
- a small bridge for future FFI work such as Rust

Current scope is intentionally small:

- endpoint create/destroy/init
- connect, tick, packet ingress, send
- connection state, stats, runtime metrics

The current header is:

- `include/rudp/rudp_c.h`

## Build and Install

```bash
cmake -S . -B build -DRUDP_INSTALL=ON -DRUDP_BUILD_TESTS=OFF
cmake --build build --config Release
cmake --install build --config Release
```

After install, the public C header is available at:

- `include/rudp/rudp_c.h`

## Minimal Example

```c
#include "rudp/rudp_c.h"

#include <stdint.h>

static uint32_t NowMs(void* user) {
    (void)user;
    return 0;
}

static int SendRaw(void* user, const uint8_t* data, uint16_t len) {
    (void)user;
    (void)data;
    return len > 0 ? 1 : 0;
}

static void OnDeliver(void* user, const uint8_t* data, uint16_t len) {
    (void)user;
    (void)data;
    (void)len;
}

int main(void) {
    rudp_endpoint_handle* handle = 0;
    rudp_config_v1 cfg;
    rudp_hooks_v1 hooks;

    rudp_default_config_v1(&cfg);
    hooks.user = 0;
    hooks.now_ms = NowMs;
    hooks.send_raw = SendRaw;
    hooks.on_deliver = OnDeliver;

    if (rudp_endpoint_create(&handle) != RUDP_STATUS_OK) return 1;
    if (rudp_endpoint_init(handle, &cfg, &hooks) != RUDP_STATUS_OK) return 2;
    if (rudp_endpoint_start_connect(handle) != RUDP_STATUS_OK) return 3;

    rudp_endpoint_tick(handle);
    rudp_endpoint_destroy(handle);
    return 0;
}
```

## State Model

Important return expectations:

- null pointers return `RUDP_STATUS_INVALID_ARGUMENT`
- functions on a created-but-not-initialized handle return `RUDP_STATUS_NOT_INITIALIZED`
- `rudp_endpoint_send(...)` returns `RUDP_STATUS_NOT_CONNECTED` until the handshake completes

## Validation Paths In This Repository

The current repository validates the C API through:

- `tests/c_api_test.cpp`: endpoint lifecycle and handshake smoke
- `tests/install_consume_c/`: installed-package consumer smoke
- `examples/c_api/install_consume/`: maintained installed-package C consumer example
- CI and `scripts/release_check.py` now execute that example as part of the ABI-sensitive validation path
- `scripts/release_check.py`: local end-to-end release verification

## Compatibility Policy

- ABI rules and allowed changes are documented in `docs/c-abi-compatibility.md`.
- New bindings should treat that page as the source of truth for what can change without an ABI bump.

## Current Limits

- the ABI is still intentionally small and not yet declared stable
- `ConnectionManager` is not exposed through the C API
- zero-copy send is not part of the C API yet
- advanced control-plane features such as key rotation are still C++-only

## Next Step

If you plan to build a Rust wrapper, use this page together with `docs/c-abi-draft.md`, `docs/c-abi-stability-assessment.md`, and `docs/c-abi-downstream-baseline-decision.md`.
The correct order is still: finish the C ABI stability decision first, then build Rust on top of that baseline.
