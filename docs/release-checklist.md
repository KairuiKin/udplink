# Release Checklist

Use this checklist before cutting the next tag.

## Validation

- `python scripts/release_check.py` passes locally.
- `python scripts/check_c_abi_surface.py` passes and matches `include/rudp/rudp_c.h` to `docs/c-abi-surface-v1.json`.
- Build succeeds with `cmake -S . -B build` and `cmake --build build --config Release`.
- `ctest --test-dir build --output-on-failure` passes on Unix-like systems when applicable.
- `ctest --test-dir build -C Release --output-on-failure` passes on Windows.
- `rudp_bench` still runs.
- Benchmark docs still match the current `tests/bench.cpp` shape.
- Benchmark release-history entries include enough metadata to compare runs responsibly.

## Packaging

- Install succeeds with `cmake --install`.
- `tests/install_consume` configures with `find_package(rudp CONFIG REQUIRED)`.
- `tests/install_consume_c` configures with `find_package(rudp CONFIG REQUIRED)`.
- `examples/c_api/install_consume` configures with `find_package(rudp CONFIG REQUIRED)`.
- Consumer build links against `rudp::rudp` successfully.
- Installed package still contains `include/rudp/rudp_c.h`.
- The maintained C API consumer example still builds and runs from the installed package path.
- On Windows, installed-package consumers still work when configured with `-Drudp_DIR:PATH=<stage>\lib\cmake\rudp`.

## Docs

- `CHANGELOG.md` reflects shipped changes.
- `README.md`, `README.en.md`, `CONTRIBUTING.md`, and `MAINTAIN.md` match the current API and validation layout.
- `docs/index.html` matches the current validation entry points and repository status.
- `docs/c-api-quickstart.md`, `docs/c-abi-draft.md`, `docs/c-abi-compatibility.md`, `docs/c-abi-stability-gate.md`, `docs/c-abi-stability-assessment.md`, `docs/c-abi-stability-decision-playbook.md`, `docs/c-abi-downstream-baseline-decision.md`, and `docs/c-abi-surface-v1.json` match the shipped C ABI surface and current stability position.
- `python scripts/check_c_abi_docs.py --mode wait` passes and confirms that the current wait-state wording is aligned across the key C ABI docs, with no stable-only wording mixed into those pages.
- Embedded examples are still clearly marked as reference templates when not CI-built.

## Release Cut

- Merge target branch into `master`/`main`.
- Confirm `CHANGELOG.md` keeps `0.1.2 - 2026-03-28` as the release section and that `Unreleased` is either empty or only contains post-release work.
- If the release claims the C ABI is stable, confirm every gate in `docs/c-abi-stability-gate.md` is met, `docs/c-abi-stability-assessment.md` is updated for that release candidate, `python scripts/evaluate_c_abi_gate.py --docs-mode stable --downstream-baseline-adopted --no-near-term-break --require-stable-ready` passes, and the release notes state that claim explicitly.
- Tag the release commit.
- Publish GitHub release notes.
