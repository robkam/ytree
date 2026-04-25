#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

cd "$PROJECT_ROOT"
source .venv/bin/activate

if ! command -v pip-compile >/dev/null 2>&1; then
  pip install pip-tools
fi

export PIP_TOOLS_CACHE_DIR="${PIP_TOOLS_CACHE_DIR:-/tmp/ytree-pip-tools-cache}"
mkdir -p "$PIP_TOOLS_CACHE_DIR"

pip-compile --upgrade -o scripts/requirements.txt scripts/requirements.in

echo "Updated scripts/requirements.txt from scripts/requirements.in"
