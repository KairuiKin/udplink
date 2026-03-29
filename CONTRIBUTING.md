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

### Unix-like

```bash
ctest --test-dir build --output-on-failure
```

### Windows PowerShell

```powershell
ctest --test-dir build -C Release --output-on-failure
```

## Docs Sync

```bash
python scripts/sync_docs_snippets.py
```


## C ABI Docs Check

```bash
python scripts/check_c_abi_docs.py --mode wait
```

## Install Consume Check

### Unix-like

```bash
cmake -S . -B build-install -DRUDP_INSTALL=ON -DRUDP_BUILD_TESTS=OFF -DRUDP_BUILD_EXAMPLES=OFF
cmake --build build-install --config Release
cmake --install build-install --prefix ./stage
cmake -S tests/install_consume -B build-consume -DCMAKE_PREFIX_PATH=./stage
cmake -S tests/install_consume_c -B build-consume-c -DCMAKE_PREFIX_PATH=./stage
cmake -S examples/c_api/install_consume -B build-example-c-api -DCMAKE_PREFIX_PATH=./stage
cmake --build build-consume --config Release
cmake --build build-consume-c --config Release
cmake --build build-example-c-api --config Release
```

### Windows PowerShell

```powershell
cmake -S . -B build-install -DRUDP_INSTALL=ON -DRUDP_BUILD_TESTS=OFF -DRUDP_BUILD_EXAMPLES=OFF
cmake --build build-install --config Release
cmake --install build-install --config Release --prefix .\stage
cmake -S tests/install_consume -B build-consume -Drudp_DIR:PATH="$PWD\stage\lib\cmake\rudp"
cmake -S tests/install_consume_c -B build-consume-c -Drudp_DIR:PATH="$PWD\stage\lib\cmake\rudp"
cmake -S examples/c_api/install_consume -B build-example-c-api -Drudp_DIR:PATH="$PWD\stage\lib\cmake\rudp"
cmake --build build-consume --config Release
cmake --build build-consume-c --config Release
cmake --build build-example-c-api --config Release
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
2. `rudp_self_test`, `rudp_reliability_test`, `rudp_manager_test`, and `rudp_c_api_test` pass when applicable.
3. Benchmark still runs (`rudp_bench`).
4. Docs snippets are synced (`python scripts/sync_docs_snippets.py`).
5. C ABI docs state check passes (`python scripts/check_c_abi_docs.py --mode wait`) unless this PR is intentionally preparing a stable declaration change set.
6. Install/export still works for `tests/install_consume`, `tests/install_consume_c`, and `examples/c_api/install_consume` when packaging or ABI-facing code changes.
7. Public API changes are reflected in README and docs.
8. If the PR touches ABI-governance or board-backed validation, use the repository issue templates to record the supporting decision or evidence.
9. Use `.github/pull_request_template.md` when opening the PR and do not leave relevant sections blank.
