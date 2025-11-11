# Contributing to ytree

Thank you for your interest in contributing to ytree! This document provides guidelines for setting up your development environment, running tests, and submitting your changes.

## Development Setup

This project uses a combination of C for the application and Python for the test suite.

### 1. Prerequisites

- A C compiler (like `gcc`) and `make`.
- `libncurses-dev`, `libtinfo-dev`, `libreadline-dev`, `libarchive-dev`.
- `python3` and `python3-venv`.
- `pandoc` (for generating the man page).

On a Debian/Ubuntu-based system, you can install these with:
```bash
sudo apt-get update
sudo apt-get install build-essential libncurses-dev libtinfo-dev libreadline-dev libarchive-dev python3-venv pandoc
```

### 2. Initial Python Environment Setup

A helper script is provided to create an isolated Python environment for testing. From the root of the project, run:

```bash
./setup_dev.sh
```

This script will create a `.venv` directory, activate it, and install the required Python libraries (`pytest`, `pexpect`).

After the first run, you must reactivate this environment for any new shell session before running tests:
```bash
source .venv/bin/activate
```

## Building the Project

The typical workflow for compiling your changes is:

```bash
# Clean previous build artifacts (optional but recommended)
make clean

# Compile the ytree executable and generate the man page
make
```

After a successful build, you can run the local executable for manual testing with `./ytree`.

## Running the Test Suite

The test suite uses `pytest` and `pexpect` to simulate user interaction and verify application behavior. Before submitting any changes, you must ensure that all automated tests pass.

To run the full suite, first ensure your Python virtual environment is activated, then run `pytest` from the project root:

```bash
# If not already activated:
# source .venv/bin/activate

pytest
```

The tests will automatically build a test environment, run the local `./ytree` binary against it, verify outcomes, and clean up.

**Important:** If you are adding a new feature or fixing a bug, please add a corresponding test case to `tests/test_main.py` that covers your changes.

## Submitting Changes

1.  **Fork the repository** on GitHub.
2.  **Create a new branch** for your feature or bugfix (`git checkout -b feature/my-new-feature`).
3.  **Make your changes** and commit them with a clear, declarative commit message.
4.  **Push your branch** to your fork (`git push origin feature/my-new-feature`).
5.  **Open a Pull Request** against the `main` branch of the upstream ytree repository.

## Coding Style

Please adhere to the existing coding style found throughout the project. The codebase follows C89/C99 standards with a focus on readability, consistency, and proper resource management.