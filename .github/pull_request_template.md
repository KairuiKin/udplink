## Summary

Describe the change and why it is needed.

## Change Type

- [ ] Core protocol / state-machine change
- [ ] Public C++ API change
- [ ] Public C ABI change
- [ ] Packaging / install-consume change
- [ ] Docs-only change
- [ ] Embedded example / board bring-up change
- [ ] Release / governance / decision-doc change

## Validation

- [ ] `rudp_self_test`
- [ ] `rudp_reliability_test`
- [ ] `rudp_manager_test`
- [ ] `rudp_c_api_test`
- [ ] `rudp_bench`
- [ ] `tests/install_consume`
- [ ] `tests/install_consume_c`
- [ ] `examples/c_api/install_consume`
- [ ] `python scripts/sync_docs_snippets.py`
- [ ] `python scripts/check_c_abi_docs.py --mode wait`
- [ ] `python scripts/release_check.py`
- [ ] Not run, with explanation below

## Docs

- [ ] README / docs updated where needed
- [ ] Public API changes reflected in docs
- [ ] C ABI decision docs remain aligned
- [ ] No wait/stable mixed wording introduced

## Special Checks

If this PR touches the C ABI surface or the repository's ABI position:

- [ ] I reviewed `docs/c-abi-stability-gate.md`
- [ ] I reviewed `docs/c-abi-stability-assessment.md`
- [ ] I reviewed `docs/c-abi-downstream-baseline-decision.md`
- [ ] I reviewed `docs/c-abi-stability-decision-playbook.md`
- [ ] I updated `python scripts/check_c_abi_docs.py` expectations if needed

If this PR claims the C ABI is stable:

- [ ] I updated docs in one coherent change set
- [ ] `python scripts/check_c_abi_docs.py --mode stable` passes
- [ ] No wait-only wording remains in official stable docs
- [ ] Release notes / changelog include the stable declaration intentionally

If this PR reports board-backed validation:

- [ ] I used the `Board Bring-Up Report` issue template or included equivalent evidence
- [ ] The run shows both sides reaching connected state and app payload delivery

## Notes

List anything not covered above: skipped checks, environment constraints, follow-ups, or risks.
