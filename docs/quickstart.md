# Quickstart

This page is for contributors and developers who want to clone the repo, set up a local environment, build, and run tests.

If you want the user manual, see [USAGE.md](USAGE.md).
If you want the full contribution workflow, see [CONTRIBUTING.md](CONTRIBUTING.md).

## 1. Prerequisites

You need:

- a C compiler, `gcc` or `clang`
- `make`
- `libncurses-dev`
- `libreadline-dev`
- `libarchive-dev`
- `python3`
- `python3-venv`
- `cmark`
- `lcov`
- `llvm-symbolizer` or equivalent LLVM tooling for sanitizer traces

On Debian or Ubuntu, the typical install set is:

```bash
sudo apt-get update
sudo apt-get install build-essential clang llvm libncurses-dev libtinfo-dev libreadline-dev libarchive-dev python3-venv cmark lcov
```

## 2. Clone the repo

```bash
git clone https://github.com/robkam/ytreenova.git
cd ytreenova
```

## 3. Create the local Python environment

```bash
scripts/setup_dev.sh
```

That script creates `.venv` in the repo root and installs the pinned Python dependencies.

Afterwards, activate it in each new shell:

```bash
source .venv/bin/activate
```

## 4. Build

```bash
make
```

For sanitizer enabled debugging:

```bash
make clean
make DEBUG=1
```

## 5. Validation

Use these commands to check the repo after changes:

```bash
pytest
make qa-code-quality
make qa-all
```

Start with focused build and test commands, then use the broader validation gate when you want the full local check.

## 6. If you use Codex or another AI client

The repository includes optional Codex related configuration under:

- `.codex/config.toml`
- `.ai/codex.md`

If you use VSCodium with the Codex extension or another AI client, the setup should be easy to adapt. The main things to check are:

- your editor points at the repo root
- your AI client can adapt `.ai/codex.md`
- your client can adapt the repo's `.codex/config.toml` or an equivalent local config

You may need to adjust the path settings in your clone to suit your setup.

## 7. Where to look next

- `docs/CONTRIBUTING.md` for the full development workflow
- `docs/ARCHITECTURE.md` for architecture constraints
- `docs/SPECIFICATION.md` for behavior requirements
- `docs/AUDIT.md` for validation rules
