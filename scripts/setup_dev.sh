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

# 0. System Dependencies (WSL/Debian/Ubuntu)
echo ">>> Checking System Dependencies..."
if command -v apt-get &> /dev/null; then
    # We are likely on Debian/Ubuntu/WSL
    MISSING_PACKAGES=""
    
    # Check for basic build tools
    if ! command -v make &> /dev/null; then MISSING_PACKAGES="$MISSING_PACKAGES build-essential"; fi
    
    # Check for libraries (checking dpkg is a rough heuristic, but works for apt systems)
    if ! dpkg -l | grep -q libncurses-dev; then MISSING_PACKAGES="$MISSING_PACKAGES libncurses-dev"; fi
    if ! dpkg -l | grep -q libreadline-dev; then MISSING_PACKAGES="$MISSING_PACKAGES libreadline-dev"; fi
    if ! dpkg -l | grep -q libarchive-dev; then MISSING_PACKAGES="$MISSING_PACKAGES libarchive-dev"; fi
    if ! dpkg -l | grep -q python3-pip; then MISSING_PACKAGES="$MISSING_PACKAGES python3-pip"; fi
    if ! dpkg -l | grep -q python3-venv; then MISSING_PACKAGES="$MISSING_PACKAGES python3-venv"; fi
    if ! dpkg -l | grep -q pandoc; then MISSING_PACKAGES="$MISSING_PACKAGES pandoc"; fi

    if [ -n "$MISSING_PACKAGES" ]; then
        echo "Installing missing packages: $MISSING_PACKAGES"
        sudo apt-get update
        sudo apt-get install -y $MISSING_PACKAGES
    else
        echo "All system dependencies appear to be installed."
    fi
else
    echo "Warning: Not on a Debian-based system. Please manually ensure you have: make, gcc, ncurses-dev, readline-dev, libarchive-dev"
fi

# 0.5. Git Configuration
echo ">>> Verifying Git Configuration..."
GIT_USER=$(git config --global user.name || echo "")
GIT_EMAIL=$(git config --global user.email || echo "")

if [ -z "$GIT_USER" ]; then
    echo "Git user.name is not set."
    read -p "Enter your Name for Git: " INPUT_NAME
    git config --global user.name "$INPUT_NAME"
fi

if [ -z "$GIT_EMAIL" ]; then
    echo "Git user.email is not set."
    read -p "Enter your Email for Git: " INPUT_EMAIL
    git config --global user.email "$INPUT_EMAIL"
fi

# 1. Check for Python 3
if ! command -v "$PYTHON_BIN" &> /dev/null; then
    echo "Error: $PYTHON_BIN could not be found."
    echo "Please install python3."
    exit 1
fi

# 2. Setup Virtual Environment
if [ -d "$VENV_DIR" ] && [ ! -f "$VENV_DIR/bin/activate" ]; then
    echo "Found broken virtual environment (missing activate script). Removing..."
    rm -rf "$VENV_DIR"
fi

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
echo "  pytest"
echo "--------------------------------------------------------"