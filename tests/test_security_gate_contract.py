from pathlib import Path


REQUIRED_BRANCH_PROTECTION_CHECKS = (
    "Docs gate",
    "Guard and code-quality gate",
    "Guard fuzz harness sync",
    "File mutation integrity gate",
    "Static analyzer gate",
    "Runtime and security gate",
    "Full pytest gate",
    "Sanitizer gate",
    "Full coverage baseline gate",
    "Fuzz baseline gate",
    "Up To Date With Main",
)


def _assert_invariant(condition: bool, invariant: str) -> None:
    assert condition, (
        f"{invariant} Runtime execution cannot safely prove that workflow, branch-policy, "
        "and template safeguards remain configured."
    )


WORKFLOW_SECURITY_INVARIANT = (
    "Security-policy invariant: required CI gates and merge-validation evidence remain configured."
)


def _read(path: str) -> str:
    return Path(path).read_text(encoding="utf-8")


def test_full_qa_workflow_retains_runtime_and_security_gate_contract() -> None:
    source = _read(".github/workflows/full-qa.yml")

    _assert_invariant("runtime-and-security:" in source, WORKFLOW_SECURITY_INVARIANT)
    _assert_invariant("name: Runtime and security gate" in source, WORKFLOW_SECURITY_INVARIANT)
    _assert_invariant("make qa-valgrind" in source, WORKFLOW_SECURITY_INVARIANT)
    _assert_invariant(
        'make qa-fuzz FUZZ_CC="ccache clang"' in source, WORKFLOW_SECURITY_INVARIANT
    )
    _assert_invariant("make qa-gitleaks" in source, WORKFLOW_SECURITY_INVARIANT)
    _assert_invariant("name: qa-runtime-security-log" in source, WORKFLOW_SECURITY_INVARIANT)


def test_docs_list_current_required_checks_for_merge_policy() -> None:
    for path in ("docs/AUDIT.md", "docs/CONTRIBUTING.md"):
        source = _read(path)
        for check_name in REQUIRED_BRANCH_PROTECTION_CHECKS:
            _assert_invariant(check_name in source, WORKFLOW_SECURITY_INVARIANT)


def test_pr_gate_and_template_require_security_validation_evidence() -> None:
    pr_gate = _read("docs/PR_GATE.md")
    template = _read(".github/pull_request_template.md")

    _assert_invariant("non-trivial PR" in pr_gate, WORKFLOW_SECURITY_INVARIANT)
    _assert_invariant(".github/workflows/full-qa.yml" in pr_gate, WORKFLOW_SECURITY_INVARIANT)
    _assert_invariant("Runtime and security gate" in pr_gate, WORKFLOW_SECURITY_INVARIANT)
    _assert_invariant("make qa-unsafe-apis" in pr_gate, WORKFLOW_SECURITY_INVARIANT)
    _assert_invariant("make qa-fileops-integrity" in pr_gate, WORKFLOW_SECURITY_INVARIANT)

    _assert_invariant("## Validation" in template, WORKFLOW_SECURITY_INVARIANT)
    _assert_invariant("make qa-unsafe-apis" in template, WORKFLOW_SECURITY_INVARIANT)
    _assert_invariant("make qa-fileops-integrity" in template, WORKFLOW_SECURITY_INVARIANT)
    _assert_invariant(".github/workflows/full-qa.yml" in template, WORKFLOW_SECURITY_INVARIANT)
