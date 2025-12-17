#!/bin/bash
# setup_dev.sh
# Sets up the development environment for ytree (Python venv + dependencies)

set -e  # Exit immediately if a command exits with a non-zero status

# Determine the project root relative to this script
# (Assuming script is in <root>/scripts/)
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# Keep the virtual environment in the root for easy access
VENV_DIR="$PROJECT_ROOT/.venv"
PYTHON_BIN="python3"
# Requirements file is now alongside this script in scripts/
REQ_FILE="$SCRIPT_DIR/requirements.txt"

# 1. Check for Python 3
if ! command -v "$PYTHON_BIN" &> /dev/null; then
    echo "Error: $PYTHON_BIN could not be found."
    echo "Please install python3 and python3-venv."
    exit 1
fi

# 2. Create Virtual Environment if it doesn't exist
if [ ! -d "$VENV_DIR" ]; then
    echo "Creating Python virtual environment in $VENV_DIR..."
    "$PYTHON_BIN" -m venv "$VENV_DIR"
else
    echo "Virtual environment already exists in $VENV_DIR."
fi

# 3. Activate and Install Dependencies
echo "Installing/Updating dependencies..."

if [ ! -f "$REQ_FILE" ]; then
    echo "Error: $REQ_FILE not found at $REQ_FILE"
    exit 1
fi

# We use a subshell to activate the venv just for installation
(
    source "$VENV_DIR/bin/activate"

    # Upgrade pip first
    pip install --upgrade pip

    # Install required packages from requirements.txt
    pip install -r "$REQ_FILE"
)

echo ""
echo "--------------------------------------------------------"
echo "Setup complete."
echo ""
echo "To start developing, activate the environment:"
echo "  source .venv/bin/activate"
echo ""
echo "Then you can run:"
echo "  scripts/chat-ytree.py"
echo "  pytest"
echo "--------------------------------------------------------"