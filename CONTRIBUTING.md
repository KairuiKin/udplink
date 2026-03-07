# Contributing

## Development Requirements

- CMake 3.10+
- C++11 compiler

## Build

```bash
cmake -S . -B build
cmake --build build --config Release
```

## Test

```bash
build/rudp_self_test
build/rudp_bench
```

## Coding Rules

- Keep C++11 compatibility.
- Avoid heap allocation in protocol hot path.
- Preserve deterministic behavior for embedded targets.
- Add tests for any protocol/state-machine change.

## Pull Request Checklist

1. Build succeeds on your target platform.
2. `rudp_self_test` passes.
3. Benchmark still runs (`rudp_bench`).
4. Public API changes are reflected in README.
