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


def _read(path: str) -> str:
    return Path(path).read_text(encoding="utf-8")


def test_full_qa_workflow_retains_runtime_and_security_gate_contract() -> None:
    source = _read(".github/workflows/full-qa.yml")

    assert "runtime-and-security:" in source
    assert "name: Runtime and security gate" in source
    assert "make qa-valgrind" in source
    assert 'make qa-fuzz FUZZ_CC="ccache clang"' in source
    assert "make qa-gitleaks" in source
    assert "name: qa-runtime-security-log" in source


def test_docs_list_current_required_checks_for_merge_policy() -> None:
    for path in ("docs/AUDIT.md", "docs/CONTRIBUTING.md"):
        source = _read(path)
        for check_name in REQUIRED_BRANCH_PROTECTION_CHECKS:
            assert check_name in source, f"{path} missing required check {check_name}"


def test_pr_gate_and_template_require_security_validation_evidence() -> None:
    pr_gate = _read("docs/PR_GATE.md")
    template = _read(".github/pull_request_template.md")

    assert "non-trivial PR" in pr_gate
    assert ".github/workflows/full-qa.yml" in pr_gate
    assert "Runtime and security gate" in pr_gate
    assert "make qa-unsafe-apis" in pr_gate
    assert "make qa-fileops-integrity" in pr_gate

    assert "## Validation" in template
    assert "make qa-unsafe-apis" in template
    assert "make qa-fileops-integrity" in template
    assert ".github/workflows/full-qa.yml" in template
