# C ABI Stable Patch Draft

Snapshot date: 2026-03-29.

This page is a draft-only patch aid for the future case where maintainers intentionally flip the repository from `wait` to `stable`.

It does not change the current repository position by itself.
Use it only when maintainers are ready to apply one coherent stable-declaration change set.

## How To Use This Draft

1. confirm the decision using `docs/c-abi-stability-decision-playbook.md`
2. use `docs/c-abi-stable-candidate-change-set.md` as the file checklist
3. use `docs/c-abi-stable-wording-pack.md` as the wording source
4. use this page as the current-to-target patch map
5. finish by making `python scripts/check_c_abi_docs.py --mode stable` pass

## `docs/c-abi-stability-assessment.md`

### Replace Current Assessment Block

Current block:

```text
Current answer: not yet.

Reason:

- the repository now has a small implemented ABI, written compatibility rules, CI-backed install-consume coverage, a maintained in-repo C consumer example, and aligned documentation
- but it still does not clearly satisfy the gate requirement for at least one credible downstream consumer story
- maintainers also have not yet written an explicit "no near-term ABI break expected" confirmation tied to a release candidate

That means the current ABI is much closer to stability than before, but the safe position is still to keep it implemented without making a formal long-term stability promise.
```

Replace with:

```text
Current answer: ready.

Reason:

- the repository now has a small implemented ABI, written compatibility rules, CI-backed install-consume coverage, a maintained in-repo C consumer example, and aligned documentation
- maintainers now explicitly treat `examples/c_api/install_consume/` as the first supported downstream baseline for the current C ABI
- maintainers do not expect a near-term breaking redesign of the current v1 C ABI baseline

That means the repository may now describe the current v1 C ABI as a stable baseline rather than an implemented draft.
```

### Replace Downstream Use Case Status

Current block:

```text
Status: not yet met.
```

Replace with:

```text
Status: met.
```

### Replace Recommended Decision Section

Current block:

```text
Recommendation: wait before declaring the current C ABI stable.
```

Replace with:

```text
Recommendation: the current C ABI baseline may be declared stable.
```

## `docs/c-abi-downstream-baseline-decision.md`

### Replace Current Status Section

Current block:

```text
Decision status: proposed, not yet adopted.

Current recommendation: do not bless it yet.
```

Replace with:

```text
Decision status: adopted.

Current recommendation: bless it.
```

### Replace Current Recommendation Body

Current intent should change from evidence-only wording to adopted-baseline wording.

Replace the current recommendation section with wording close to:

```text
Recommendation today: choose Option A.

Why:

- the maintained example is already exercised by CI and `scripts/release_check.py`
- maintainers now explicitly want that example to carry compatibility weight as the first supported downstream baseline
- maintainers do not expect a near-term breaking redesign of the current v1 C ABI
```

## `docs/c-abi-compatibility.md`

### Replace Current Status Sentence

Current block:

```text
The C ABI is implemented, but it is not yet declared long-term stable across arbitrary future releases.
```

Replace with:

```text
The current v1 C ABI is implemented and is now treated as an explicitly stable baseline across future releases unless a new ABI revision is intentionally announced.
```

### Add No-Break Confirmation

Add wording close to:

```text
Maintainers do not expect a near-term breaking redesign of this baseline. Future ABI-sensitive changes should therefore remain additive unless a new ABI revision is intentionally announced.
```

## `docs/c-api-quickstart.md`

### Replace Current Limits Block

Current block:

```text
- the ABI is still intentionally small and not yet declared stable
- `ConnectionManager` is not exposed through the C API
- zero-copy send is not part of the C API yet
- advanced control-plane features such as key rotation are still C++-only
```

Replace with:

```text
- the current C ABI baseline is now treated as stable
- the surface remains intentionally small even though it is now stable
- `ConnectionManager` is not exposed through the C API
- zero-copy send is not part of the C API yet
- advanced control-plane features such as key rotation are still C++-only
```

## `docs/project-status.md`

### Replace Open-Items Block

Current block:

```text
- the current C ABI is implemented, but it is not yet declared stable
- maintainers have not yet blessed `examples/c_api/install_consume/` as the first supported downstream baseline
- there is not yet an explicit no-break statement for the current v1 C ABI baseline
```

Replace with:

```text
- the current C ABI baseline is declared stable
- `examples/c_api/install_consume/` is the first supported downstream baseline
- maintainers do not expect a near-term breaking redesign of the current v1 C ABI baseline
```

### Adjust Recommended Route

Current intent says Rust must wait until the ABI gate is met.

Replace that intent with wording close to:

```text
3. Start a Rust wrapper spike only on top of `rudp/rudp_c.h`.
   - the stable baseline is now available for wrapper work
   - any missing pieces should still feed back as focused additive ABI extensions
```

## `docs/c-abi-stability-decision-playbook.md`

### Replace Current Recommendation Line

Current block:

```text
Current recommended answer: keep `wait`.
```

Replace with:

```text
Current recommended answer: declare `stable`.
```

### Replace Wait-Mode Validation Note

Current block:

```text
- `python scripts/check_c_abi_docs.py --mode wait` still passes
- no stable-only wording has leaked into the official wait-state docs
```

Replace with:

```text
- `python scripts/check_c_abi_docs.py --mode stable` still passes
- no wait-only wording remains in the official stable docs
```

## `docs/release-checklist.md`

### Replace Docs-State Check Line

Current block:

```text
- `python scripts/check_c_abi_docs.py --mode wait` passes and confirms that the current wait-state wording is aligned across the key C ABI docs, with no stable-only wording mixed into those pages.
```

Replace with:

```text
- `python scripts/check_c_abi_docs.py --mode stable` passes and confirms that the stable-state wording is aligned across the key C ABI docs, with no wait-only wording left behind.
```

## `scripts/check_c_abi_docs.py`

### Mode Switch Follow-Through

After applying the official stable wording to the docs above:

- keep `--mode wait` support for historical use and branch comparisons
- update the default release-facing path to use `--mode stable` where appropriate
- confirm `--mode stable` passes and `--mode wait` fails against the new official stable docs

## Final Sanity Rule

Do not treat this page as permission to merge a partial stable declaration.

Use it only if all of the following happen together:

- official docs are updated coherently
- release notes and changelog make the stable declaration intentionally
- `python scripts/check_c_abi_docs.py --mode stable` passes
- no wait-only wording remains in the official stable pages
```
