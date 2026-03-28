# Benchmarks

This page describes the current `rudp_bench` smoke benchmark and records a local sample run.

## What `rudp_bench` measures

- In-memory endpoint-to-endpoint transfer with no real socket I/O.
- Fixed payload size: `96` bytes.
- Fixed message count: `20000`.
- Auth enabled.
- Mid-run key rotation enabled.
- Alternating `Send()` and `SendZeroCopy()` calls.

The benchmark is useful for regression tracking inside the protocol core. It is not a network throughput claim.

## Source of truth

- Benchmark implementation: `tests/bench.cpp`
- CI smoke execution: `.github/workflows/ci.yml`

## Run locally

```bash
cmake -S . -B build
cmake --build build --config Release
build/rudp_bench
```

## Local sample

Environment:

- Date: `2026-03-28`
- Host: local Windows development machine
- Build: `Release`

Sample output:

```text
rudp bench: messages=20000 payload=96B time=0.033s msg/s=606060.6 MB/s=55.49
```

## How to interpret results

- Compare results only against the same benchmark shape.
- Expect variation across compiler, platform, and build type.
- Use this benchmark to catch large regressions, not to market headline throughput.

## Next improvements

- Add Linux sample data alongside Windows sample data.
- Record compiler and CPU details with each published sample.
- Add a scripted benchmark archive if multiple releases need comparison.
