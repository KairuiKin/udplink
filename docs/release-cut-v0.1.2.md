# Release Cut Playbook: v0.1.2

Use this sequence when turning the current work into an actual release.

## 1. Final local verification

- run `python scripts/release_check.py`
- inspect `git diff` and confirm no accidental edits remain
- confirm release docs still match the current code and CI setup

## 2. Prepare release branch / PR

- create a release branch such as `release/v0.1.2`
- use `docs/pr-v0.1.2.md` as the PR body draft
- ensure CI passes on the release PR, including `install-consume`

## 3. Final changelog move

- confirm the `0.1.2 - 2026-03-28` section in `CHANGELOG.md` matches the final merged state
- keep any last-minute edits grouped under `Added`, `Changed`, and `Fixed`

## 4. Merge and tag

- merge the release PR into `master`/`main`
- create tag `v0.1.2` on the merge commit

## 5. Publish release

- use `docs/github-release-v0.1.2.md` as the GitHub release body
- attach any benchmark or packaging notes only if they match the final merged state

## 6. Post-release sanity

- verify the tag points to the merged release commit
- verify all GitHub Actions checks are green on the tagged commit
- confirm the release page links and docs navigation render correctly
