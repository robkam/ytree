#!/bin/bash
# setup_dev.sh
# Sets up the development environment for ytree (Python venv + dependencies)

set -e  # Exit immediately if a command exits with a non-zero status

VENV_DIR=".venv"
PYTHON_BIN="python3"

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

# We use a subshell to activate the venv just for installation
(
    source "$VENV_DIR/bin/activate"

    # Upgrade pip first
    pip install --upgrade pip

    # Install required packages
    # google-generativeai: For build-ytree.py and chat-ytree.py
    # prompt_toolkit: For enhanced input in chat-ytree.py
    # pytest, pexpect: For the test suite
    pip install google-generativeai prompt_toolkit pytest pexpect
)

echo ""
echo "--------------------------------------------------------"
echo "Setup complete."
echo ""
echo "To start developing, activate the environment:"
echo "  source $VENV_DIR/bin/activate"
echo ""
echo "Then you can run:"
echo "  ./chat-ytree.py"
echo "  ./build-ytree.py --task task.txt"
echo "  pytest"
echo "--------------------------------------------------------"