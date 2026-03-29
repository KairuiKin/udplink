# Minimal C ABI Draft

## Status

This document now tracks an initial implementation skeleton, not just a paper design.

Current repository state:

- public C header: `include/rudp/rudp_c.h`
- wrapper translation unit: `src/rudp_c.cpp`
- smoke coverage: `tests/c_api_test.cpp`

The ABI is still not declared stable. Its purpose is to define and constrain the smallest C-facing surface that would let other languages, especially Rust, integrate with `udplink` without binding directly to the current C++ classes.

## Goals

- keep the first ABI small enough to stabilize
- wrap `Endpoint` before attempting to wrap `ConnectionManager`
- expose only lifecycle, packet ingress/egress, tick driving, connection state, and metrics
- make ownership and callback rules explicit for FFI users
- keep the implementation free to evolve behind an opaque handle

## Non-Goals

- exposing every C++ method immediately
- promising zero-copy semantics across the ABI boundary
- exposing the manager layer in v1 of the C ABI
- async runtime integration
- `no_std` or embedded Rust commitments in the first iteration

## Design Principles

### Opaque Handles

The C ABI should expose opaque pointer types only:

```c
typedef struct rudp_endpoint_handle rudp_endpoint_handle;
```

The actual C++ object layout stays private to the library.

### Flat Value Types

Public ABI structs should use only fixed-width integer types and POD layout.

Example categories:

- config
- stats
- runtime metrics
- hook table
- version info

### Explicit Result Codes

Do not expose C++ exceptions or enum classes directly.
Every fallible operation returns an integer status code.

Suggested baseline:

```c
typedef enum rudp_status {
    RUDP_STATUS_OK = 0,
    RUDP_STATUS_INVALID_ARGUMENT = 1,
    RUDP_STATUS_NOT_INITIALIZED = 2,
    RUDP_STATUS_QUEUE_FULL = 3,
    RUDP_STATUS_PAYLOAD_TOO_LARGE = 4,
    RUDP_STATUS_NOT_CONNECTED = 5,
    RUDP_STATUS_UNSUPPORTED = 6,
    RUDP_STATUS_INTERNAL_ERROR = 255
} rudp_status;
```

### ABI-First Callback Rules

Callbacks must be expressed in C function pointers with a `void* user` context.
No callback may require C++ object ownership on the foreign side.

## Proposed v1 Surface

### Config and Hooks

```c
typedef uint32_t (*rudp_now_ms_fn)(void* user);
typedef int (*rudp_send_raw_fn)(void* user, const uint8_t* data, uint16_t len);
typedef void (*rudp_on_deliver_fn)(void* user, const uint8_t* data, uint16_t len);

typedef struct rudp_hooks_v1 {
    void* user;
    rudp_now_ms_fn now_ms;
    rudp_send_raw_fn send_raw;
    rudp_on_deliver_fn on_deliver;
} rudp_hooks_v1;
```

`send_raw_vec` should stay out of the first ABI draft unless there is strong evidence that foreign-language users need it immediately.

### Lifecycle

```c
rudp_status rudp_endpoint_create(rudp_endpoint_handle** out_handle);
void rudp_endpoint_destroy(rudp_endpoint_handle* handle);
rudp_status rudp_endpoint_init(rudp_endpoint_handle* handle,
                               const rudp_config_v1* config,
                               const rudp_hooks_v1* hooks);
rudp_status rudp_endpoint_start_connect(rudp_endpoint_handle* handle);
rudp_status rudp_endpoint_disconnect(rudp_endpoint_handle* handle);
```

### Data Path

```c
rudp_status rudp_endpoint_on_udp_packet(rudp_endpoint_handle* handle,
                                        const uint8_t* data,
                                        uint16_t len);
rudp_status rudp_endpoint_tick(rudp_endpoint_handle* handle);
rudp_status rudp_endpoint_send(rudp_endpoint_handle* handle,
                               const uint8_t* payload,
                               uint16_t len);
```

### State and Metrics

```c
typedef enum rudp_connection_state {
    RUDP_CONNECTION_DISCONNECTED = 0,
    RUDP_CONNECTION_CONNECTING = 1,
    RUDP_CONNECTION_CONNECTED = 2,
    RUDP_CONNECTION_TIMED_OUT = 3
} rudp_connection_state;

rudp_status rudp_endpoint_is_connected(const rudp_endpoint_handle* handle,
                                       int* out_connected);
rudp_status rudp_endpoint_get_connection_state(const rudp_endpoint_handle* handle,
                                               uint8_t* out_state);
rudp_status rudp_endpoint_get_stats(const rudp_endpoint_handle* handle,
                                    rudp_stats_v1* out_stats);
rudp_status rudp_endpoint_get_runtime_metrics(const rudp_endpoint_handle* handle,
                                              rudp_runtime_metrics_v1* out_metrics);
```

## Ownership and Lifetime Rules

- `rudp_endpoint_create` allocates the handle; `rudp_endpoint_destroy` releases it.
- callbacks are borrowed for the lifetime of `init`-ed endpoint usage; caller must keep `user` valid.
- inbound packet buffers and send payload buffers are borrowed only for the duration of the call.
- stats and metric output buffers are caller-owned.
- no function returns pointers into internal library state.

## Threading Model

The first C ABI should document a single-owner threading rule:

- one endpoint handle is driven by one logical owner at a time
- concurrent `tick`, `send`, `on_udp_packet`, and `disconnect` on the same handle are not supported unless the caller serializes them
- if a foreign runtime needs multi-threaded access, it should provide external synchronization

This keeps the first ABI aligned with the current library behavior and avoids inventing a false thread-safety promise.

## Versioning Rules

### Symbol Stability

- ABI symbols should use a stable `rudp_` prefix
- the first implementation should export only v1 structs and functions
- incompatible struct changes require new suffixed types such as `rudp_config_v2`

### Runtime Version Query

Add a small runtime version entry point early:

```c
const char* rudp_version_string(void);
uint32_t rudp_abi_version(void);
```

That gives bindings a way to check compatibility explicitly.

## Testing Strategy

The C ABI should not be accepted as stable without dedicated tests.

Already present:

- smoke path for create/init/connect/send/query/disconnect in `tests/c_api_test.cpp`
- invalid-argument, bad-config, bad-hooks, and not-initialized return checks in `tests/c_api_test.cpp`
- installed-package consumer coverage in `tests/install_consume_c/`

Still required before declaring ABI stability:

1. create/init/destroy smoke across invalid-argument cases
2. active connect handshake against the existing C++ test peer
3. send and deliver one payload under explicit pass/fail checks
4. state query and runtime metrics export
5. install/consume coverage for the public C header in packaged output

Only after those tests exist should any Rust wrapper be attempted.

## Implementation Order

1. define public C header under `include/rudp/`
2. implement a thin wrapper translation unit that owns a private `rudp::Endpoint`
3. add C-oriented tests alongside existing self and manager tests
4. document status/ownership/threading rules in the README or docs
5. only then prototype a Rust wrapper on top of the stable C layer

## Open Questions

- should `send_raw_vec` appear in ABI v1 or wait until a real foreign-language consumer needs it?
- should config profiles be exposed directly, or should bindings fill the full config struct explicitly?
- should the first C ABI expose auth key rotation, or defer advanced control-plane features until v2?

## Recommendation

Do not implement Rust bindings before this draft is promoted from "initial skeleton" to a supported ABI with:

- broader ABI tests beyond the current smoke path
- written compatibility policy (`docs/c-abi-compatibility.md`)
- clear versioning rules for future struct/function growth

Until then, the right engineering move is to keep Rust as a deferred roadmap item rather than an active coding track.
