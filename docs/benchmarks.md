# Benchmarks

This page documents the current `rudp_bench` smoke benchmark and keeps a small release-to-release archive of sample outputs.

## What `rudp_bench` measures

- In-memory endpoint-to-endpoint transfer with no real socket I/O.
- Default payload size: `96` bytes; override with `rudp_bench <messages> <payload_bytes>`.
- Default message count: `20000`; override with `rudp_bench <messages> <payload_bytes>`.
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
# longer local profiling run:
build/rudp_bench 2000000 96
```

## Recording Rules

Each archived sample should record:

- release or commit identifier
- date
- platform / OS
- build type
- benchmark output line

Only compare samples that keep the same benchmark shape from `tests/bench.cpp`.

## Release History

| Release | Date | Platform | Build | Sample Output |
|---------|------|----------|-------|---------------|
| `v0.1.2` | `2026-03-28` | Windows local dev machine | `Release` | `rudp bench: messages=20000 payload=96B time=0.033s msg/s=606060.6 MB/s=55.49` |
| `master` | `2026-03-30` | Windows local dev machine | `Release` | `rudp bench: messages=20000 payload=96B time=0.028s msg/s=714285.7 MB/s=65.39` |

## Current Sample Notes

- The `v0.1.2` and `master` samples are local Windows runs, not cross-platform medians.
- The `2026-03-30` `master` sample is not directly comparable to older archive lines unless you account for the benchmark harness cleanup that removed avoidable packet copies inside `tests/bench.cpp`.
- CI currently verifies that `rudp_bench` runs, but does not archive performance outputs automatically.
- Future entries should stay conservative and avoid mixing unlike environments.

## How to interpret results

- Compare results only against the same benchmark shape.
- Expect variation across compiler, platform, and build type.
- Use this benchmark to catch large regressions, not to market headline throughput.

## Next improvements

- Add Linux sample data alongside Windows sample data.
- Record compiler and CPU details with each published sample.
- Add a scripted benchmark archive if multiple releases need comparison.
