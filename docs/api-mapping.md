# API Mapping

This page captures the current public API shape used by `udplink` examples and documents the main drift points that older examples tended to hit.

## Current endpoint integration shape

The current endpoint wiring is:

- Build a `rudp::Config` with `DefaultConfig()` or `ConfigForProfile(...)`.
- Fill `rudp::Hooks` with `now_ms`, `send_raw`, optional `send_raw_vec`, and `on_deliver`.
- Call `Endpoint::Init(config, hooks)`.
- Call `Endpoint::StartConnect()` for active open.
- Feed inbound UDP datagrams to `Endpoint::OnUdpPacket(...)`.
- Call `Endpoint::Tick()` periodically.

## Common mapping table

| Older example idea | Current API |
|--------------------|-------------|
| `get_tick_ms` | `Hooks::now_ms` |
| `send_udp` / `send` callback | `Hooks::send_raw` |
| payload callback such as `on_message` | `Hooks::on_deliver` |
| `StartConnect(peer, port)` | configure peer in your transport layer, then call `StartConnect()` |
| boolean send result | `SendStatus`, check for `SendStatus::kOk` |
| old connection state callback enums | poll `GetConnectionState()` or `IsConnected()` |
| direct payload-copy send only | optionally use `SendZeroCopy()` with `send_raw_vec` |
| custom runtime counters | use `GetStats()` and `GetRuntimeMetrics()` |

## Transport responsibility split

`udplink` does not own socket addresses. Your transport layer is responsible for:

- peer IP/port selection
- socket open/bind
- raw UDP send implementation
- inbound UDP receive loop
- scheduler/timer source

That is why the modern API keeps `StartConnect()` argument-free.

## Minimal example pattern

```cpp
rudp::Config cfg = rudp::ConfigForProfile(rudp::ConfigProfile::kBalanced);
rudp::Hooks hooks = {user, NowMs, SendRaw, SendRawVec, OnDeliver, 0, 0};

rudp::Endpoint endpoint;
if (!endpoint.Init(cfg, hooks)) {
    return 1;
}
if (!endpoint.StartConnect()) {
    return 2;
}
```

## Example maintenance rule

If an example introduces a callback or method name not present in `include/rudp/rudp.hpp`, treat it as drift and update the example before merging.
