#!/bin/bash
#
# setup_dev.sh - A script to initialize the Python development environment for ytree.

set -e

# Check if Python 3 is available
if ! command -v python3 &> /dev/null
then
    echo "Error: python3 is not installed or not in your PATH."
    exit 1
fi

# Check if venv module is available
if ! python3 -c "import venv" &> /dev/null
then
    echo "Error: The python3-venv package is not installed."
    echo "Please install it with: sudo apt-get install python3-venv"
    exit 1
fi

VENV_DIR=".venv"

if [ -d "$VENV_DIR" ]; then
    echo "Virtual environment '$VENV_DIR' already exists."
else
    echo "Creating Python virtual environment in '$VENV_DIR'..."
    python3 -m venv "$VENV_DIR"
fi

echo "Activating the virtual environment..."
source "$VENV_DIR/bin/activate"

echo "Installing/updating dependencies from requirements.txt..."
pip install -r requirements.txt

echo ""
echo "Development environment is ready."
echo "To activate it in the future, run: source $VENV_DIR/bin/activate"