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
build/rudp_reliability_test
build/rudp_bench
build/rudp_manager_test
```

## Docs Sync

```bash
python scripts/sync_docs_snippets.py
```

## Install Consume Check

```bash
cmake -S . -B build-install -DRUDP_INSTALL=ON -DRUDP_BUILD_TESTS=OFF -DRUDP_BUILD_EXAMPLES=OFF
cmake --build build-install --config Release
cmake --install build-install --prefix ./stage
cmake -S tests/install_consume -B build-consume -DCMAKE_PREFIX_PATH=./stage
cmake --build build-consume --config Release
```

## One-Command Release Check

```bash
python scripts/release_check.py
```

## Coding Rules

- Keep C++11 compatibility.
- Avoid heap allocation in protocol hot path.
- Preserve deterministic behavior for embedded targets.
- Add tests for any protocol/state-machine change.

## Pull Request Checklist

1. Build succeeds on your target platform.
2. `rudp_self_test` passes.
3. `rudp_reliability_test` passes.
4. `rudp_manager_test` passes.
5. Benchmark still runs (`rudp_bench`).
6. Docs snippets are synced (`python scripts/sync_docs_snippets.py`).
7. Install/export still works (`tests/install_consume`).
8. Public API changes are reflected in README.
