# Release Checklist

Use this checklist before cutting the next tag.

## Validation

- `python scripts/release_check.py` passes locally.
- Build succeeds with `cmake -S . -B build` and `cmake --build build --config Release`.
- `rudp_self_test` passes.
- `rudp_reliability_test` passes.
- `rudp_manager_test` passes.
- `rudp_bench` still runs.
- Benchmark docs still match the current `tests/bench.cpp` shape.

## Packaging

- Install succeeds with `cmake --install`.
- `tests/install_consume` configures with `find_package(rudp CONFIG REQUIRED)`.
- Consumer build links against `rudp::rudp` successfully.

## Docs

- `CHANGELOG.md` reflects shipped changes.
- `README.md`, `README.en.md`, and `CONTRIBUTING.md` match the current API and test layout.
- Embedded examples are still clearly marked as reference templates when not CI-built.

## Release Cut

- Merge target branch into `master`/`main`.
- Confirm `CHANGELOG.md` keeps `0.1.2 - 2026-03-28` as the release section and that `Unreleased` is either empty or only contains post-release work.
- Tag the release commit.
- Publish GitHub release notes.
