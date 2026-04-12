#!/usr/bin/env bash

set -euo pipefail

usage() {
  cat <<'EOF'
Usage: ./scripts/tether-update.sh [options]

Update or install Tether into a local source checkout + virtualenv.

Options:
  --src <path>          Tether source path (default: ~/.local/src/Tether)
  --venv <path>         Virtualenv path (default: <src>/.venv)
  --db <path>           Shared Tether DB path (default: ~/.local/share/tether/postoffice.db)
  --repo-url <url>      Tether git URL (default: https://github.com/latentcollapse/Tether.git)
  --ref <name>          Checkout this git ref before install (tag/branch/SHA)
  --no-db-backup        Skip backup of the shared DB before update
  -h, --help            Show this help and exit

Examples:
  ./scripts/tether-update.sh
  ./scripts/tether-update.sh --ref v1.6.0
  ./scripts/tether-update.sh --src "$HOME/.local/src/Tether" --db "$HOME/.local/share/tether/postoffice.db"
EOF
}

require_cmd() {
  local cmd="$1"
  if ! command -v "$cmd" >/dev/null 2>&1; then
    printf 'Error: required command not found: %s\n' "$cmd" >&2
    exit 1
  fi
}

expand_home() {
  local p="$1"
  if [[ "$p" == "~" ]]; then
    printf '%s\n' "$HOME"
    return
  fi
  if [[ "$p" == ~/* ]]; then
    printf '%s/%s\n' "$HOME" "${p#~/}"
    return
  fi
  printf '%s\n' "$p"
}

TETHER_SRC_DEFAULT="$HOME/.local/src/Tether"
TETHER_SRC="$TETHER_SRC_DEFAULT"
TETHER_VENV=""
TETHER_VENV_EXPLICIT=0
TETHER_DB="$HOME/.local/share/tether/postoffice.db"
TETHER_REPO_URL="https://github.com/latentcollapse/Tether.git"
TETHER_REF=""
DO_DB_BACKUP=1

while [[ $# -gt 0 ]]; do
  case "$1" in
    --src)
      TETHER_SRC="$(expand_home "${2:-}")"
      shift 2
      ;;
    --venv)
      TETHER_VENV="$(expand_home "${2:-}")"
      TETHER_VENV_EXPLICIT=1
      shift 2
      ;;
    --db)
      TETHER_DB="$(expand_home "${2:-}")"
      shift 2
      ;;
    --repo-url)
      TETHER_REPO_URL="${2:-}"
      shift 2
      ;;
    --ref)
      TETHER_REF="${2:-}"
      shift 2
      ;;
    --no-db-backup)
      DO_DB_BACKUP=0
      shift
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      printf 'Error: unknown option: %s\n\n' "$1" >&2
      usage >&2
      exit 1
      ;;
  esac
done

if [[ -z "$TETHER_SRC" ]]; then
  printf 'Error: --src cannot be empty.\n' >&2
  exit 1
fi
if [[ -z "$TETHER_DB" ]]; then
  printf 'Error: --db cannot be empty.\n' >&2
  exit 1
fi
if [[ -z "$TETHER_REPO_URL" ]]; then
  printf 'Error: --repo-url cannot be empty.\n' >&2
  exit 1
fi

if [[ "$TETHER_VENV_EXPLICIT" -eq 0 ]]; then
  TETHER_VENV="$TETHER_SRC/.venv"
fi

require_cmd git
require_cmd python3

mkdir -p "$(dirname "$TETHER_SRC")"
mkdir -p "$(dirname "$TETHER_DB")"

if [[ ! -d "$TETHER_SRC/.git" ]]; then
  printf '[tether-update] Cloning %s -> %s\n' "$TETHER_REPO_URL" "$TETHER_SRC"
  git clone "$TETHER_REPO_URL" "$TETHER_SRC"
else
  printf '[tether-update] Updating existing repo at %s\n' "$TETHER_SRC"
  git -C "$TETHER_SRC" fetch --all --tags --prune
fi

if [[ -n "$TETHER_REF" ]]; then
  printf '[tether-update] Checking out ref: %s\n' "$TETHER_REF"
  git -C "$TETHER_SRC" checkout "$TETHER_REF"
  if git -C "$TETHER_SRC" symbolic-ref -q HEAD >/dev/null 2>&1; then
    git -C "$TETHER_SRC" pull --ff-only
  fi
else
  git -C "$TETHER_SRC" pull --ff-only
fi

if [[ "$DO_DB_BACKUP" -eq 1 && -f "$TETHER_DB" ]]; then
  ts="$(date +%Y%m%d-%H%M%S)"
  backup_dir="$(dirname "$TETHER_DB")/backups"
  backup_path="$backup_dir/postoffice.db.$ts.bak"
  mkdir -p "$backup_dir"
  cp "$TETHER_DB" "$backup_path"
  printf '[tether-update] Backed up DB -> %s\n' "$backup_path"
fi

printf '[tether-update] Ensuring virtualenv at %s\n' "$TETHER_VENV"
python3 -m venv "$TETHER_VENV"

printf '[tether-update] Installing Tether editable package\n'
"$TETHER_VENV/bin/pip" install --upgrade pip
"$TETHER_VENV/bin/pip" install -e "$TETHER_SRC"

printf '[tether-update] Installed version: '
"$TETHER_VENV/bin/python" -c "import importlib.metadata as m; print(m.version('tether'))"
printf '[tether-update] Module path: '
"$TETHER_VENV/bin/python" -c "import tether; print(tether.__file__)"

printf '\nDone.\n'
printf 'DB path: %s\n' "$TETHER_DB"
printf 'Restart Codex/Claude/Gemini clients to reload MCP server config.\n'
