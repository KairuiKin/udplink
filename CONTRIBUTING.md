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
build/rudp_manager_test
```

## Docs Sync

```bash
python scripts/sync_docs_snippets.py
```

## Coding Rules

- Keep C++11 compatibility.
- Avoid heap allocation in protocol hot path.
- Preserve deterministic behavior for embedded targets.
- Add tests for any protocol/state-machine change.

## Pull Request Checklist

1. Build succeeds on your target platform.
2. `rudp_self_test` passes.
3. `rudp_manager_test` passes.
4. Benchmark still runs (`rudp_bench`).
5. Docs snippets are synced (`python scripts/sync_docs_snippets.py`).
6. Public API changes are reflected in README.
