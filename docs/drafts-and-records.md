# Drafts and Records

This page groups maintainer-only planning docs, release records, and draft wording packs that still matter for traceability but should not dominate first-read navigation.

Use the public landing page and the core docs first.
Use this page when you are cutting a release, preparing a decision, or tracing earlier maintainer work.

## Stable Declaration Draft Set

These files support the future case where maintainers intentionally flip the current C ABI from `wait` to `stable`.
They are draft helpers, not the current official repository stance.

- [C ABI Stable Candidate Change Set](c-abi-stable-candidate-change-set.md): the file-by-file draft of what would need to change.
- [C ABI Stable Candidate Wording Pack](c-abi-stable-wording-pack.md): wording blocks for the eventual stable declaration path.
- [PR Draft: C ABI Stable Declaration Candidate](pr-c-abi-stable-candidate.md): draft pull-request body for that future declaration.
- [C ABI Stable Patch Draft](c-abi-stable-patch-draft.md): patch-oriented helper for assembling the final stable-state edits.

## Release v0.1.2 Record Set

These pages are release-operation records for the already-cut `v0.1.2` line.
Keep them for traceability and for future release-process comparison, not as front-door docs.

- [v0.1.2 Release Plan](release-v0.1.2.md)
- [PR Draft: v0.1.2 Stabilization](pr-v0.1.2.md)
- [GitHub Release Draft: v0.1.2](github-release-v0.1.2.md)
- [Release Cut Playbook: v0.1.2](release-cut-v0.1.2.md)
- [Release Checklist](release-checklist.md)

## Backlog and Follow-Up Records

These pages capture planning state around post-release and board-validation follow-up.

- [Post-v0.1.2 Backlog](issue-drafts-post-v0.1.2.md)
- [Board Bring-Up Report Example](board-bringup-report-example.md)
- [Arduino Mega Maintainer Run](arduino-mega-w5100-maintainer-run.md)

## Use This Page When

- you are preparing or reviewing a release cut
- you are tracing the current C ABI wait-versus-stable decision history
- you need draft wording rather than the current official decision pages
- you are collecting evidence for maintainer-only operational work
