# C ABI Stable Candidate Wording Pack

Snapshot date: 2026-03-28.

This page is a draft-only wording pack for the future case where maintainers intentionally flip the repository from `wait` to `stable`.

It does not change the current repository position by itself.
Use it only as a ready-to-apply wording source for the final stable declaration change set.

## How To Use This Draft

When maintainers choose `stable`:

1. use `docs/c-abi-stable-candidate-change-set.md` as the file checklist
2. use this page as the wording source
3. update `python scripts/check_c_abi_docs.py --mode stable` in the same change set
4. do not leave old wait-only wording behind

## `docs/c-abi-stability-assessment.md`

Use wording close to this in the top-level assessment section:

```text
Current answer: ready.

Reason:

- the repository now has a small implemented ABI, written compatibility rules, CI-backed install-consume coverage, a maintained in-repo C consumer example, and aligned documentation
- maintainers now explicitly treat `examples/c_api/install_consume/` as the first supported downstream baseline for the current C ABI
- maintainers do not expect a near-term breaking redesign of the current v1 C ABI baseline

That means the repository may now describe the current v1 C ABI as a stable baseline rather than an implemented draft.
```

Use wording close to this for the recommendation section:

```text
Recommendation: the current C ABI baseline may be declared stable.
```

## `docs/c-abi-downstream-baseline-decision.md`

Use wording close to this in the status section:

```text
Decision status: adopted.

Current recommendation: bless it.
```

Use wording close to this in the decision body:

```text
`examples/c_api/install_consume/` is now the repository's first supported downstream baseline for the current C ABI.

The repository now treats this example as more than a demo:

- it is a supported reference consumer for the current C ABI
- ABI-sensitive changes should keep this example working unless a deliberate ABI revision is announced
- CI and release-check failures in this example are treated as compatibility regressions
```

## `docs/c-abi-compatibility.md`

Use wording close to this near the status/contract section:

```text
The current v1 ABI is now treated as an explicitly stable baseline.

Maintainers do not expect a near-term breaking redesign of this baseline.
Future ABI-sensitive changes should therefore remain additive unless a new ABI revision is intentionally announced.
```

## `docs/c-api-quickstart.md`

Replace the current not-yet-stable limit with wording close to this:

```text
- the current C ABI baseline is now treated as stable
- the surface remains intentionally small even though it is now stable
- `ConnectionManager` is still not exposed through the C API
- zero-copy send is still not part of the C API yet
- advanced control-plane features such as key rotation are still C++-only
```

## `docs/project-status.md`

Use wording close to this in the current/open sections:

```text
- the current C ABI baseline is declared stable
- `examples/c_api/install_consume/` is the first supported downstream baseline
- Rust work may now proceed as a wrapper strictly on top of `rudp/rudp_c.h`
```

And remove wording that still says:

```text
- the current C ABI is implemented, but it is not yet declared stable
- maintainers have not yet blessed `examples/c_api/install_consume/` as the first supported downstream baseline
- there is not yet an explicit no-break statement for the current v1 C ABI baseline
```

## `docs/c-abi-stability-decision-playbook.md`

Use wording close to this near the top:

```text
Current recommended answer: declare `stable`.
```

Use wording close to this in the validation note:

```text
- `python scripts/check_c_abi_docs.py --mode stable` still passes
- no wait-only wording remains in the official stable docs
```

## `docs/release-checklist.md`

Use wording close to this in the docs section:

```text
- `python scripts/check_c_abi_docs.py --mode stable` passes and confirms that the stable-state wording is aligned across the key C ABI docs, with no wait-only wording left behind.
```

## Release Notes / Changelog

Use wording close to this in release-facing docs:

```text
The current v1 `udplink` C ABI is now treated as a stable baseline. The maintained installed-package C consumer in `examples/c_api/install_consume/` is the repository's first supported downstream reference consumer for that ABI.
```

## Maintainer No-Break Statement

Use wording close to this in maintain or policy docs:

```text
Maintainers do not expect a near-term breaking redesign of the current v1 C ABI. Future ABI-sensitive changes should therefore remain additive unless a new ABI revision is intentionally announced.
```

## Merge Discipline

If maintainers use this wording pack, they should still treat it as one coherent change set:

- docs state changes
- release wording changes
- `scripts/check_c_abi_docs.py --mode stable`
- no leftover wait-only wording in official stable pages
```
