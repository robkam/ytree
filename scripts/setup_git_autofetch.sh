#!/bin/bash
# setup_git_autofetch.sh
# Sets up a systemd user timer to automatically git fetch origin every 5 minutes.

set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
SYSTEMD_USER_DIR="$HOME/.config/systemd/user"

echo ">>> Setting up Git Auto-Fetch for $PROJECT_ROOT"

# Ensure systemd user directory exists
if [ ! -d "$SYSTEMD_USER_DIR" ]; then
    echo "Creating systemd user directory: $SYSTEMD_USER_DIR"
    mkdir -p "$SYSTEMD_USER_DIR"
fi

# Define Service File Path
SERVICE_FILE="$SYSTEMD_USER_DIR/ytnova-autofetch.service"
TIMER_FILE="$SYSTEMD_USER_DIR/ytnova-autofetch.timer"

# Create Service File
echo "Creating service file at $SERVICE_FILE..."
cat <<EOF > "$SERVICE_FILE"
[Unit]
Description=Fetch git remotes for ytnova
After=network.target

[Service]
Type=oneshot
WorkingDirectory=$PROJECT_ROOT
ExecStart=/usr/bin/git fetch origin
EOF

# Create Timer File
echo "Creating timer file at $TIMER_FILE..."
cat <<EOF > "$TIMER_FILE"
[Unit]
Description=Run ytnova git fetch every 5 minutes

[Timer]
OnBootSec=5min
OnUnitActiveSec=5min
Unit=ytnova-autofetch.service

[Install]
WantedBy=timers.target
EOF

# Reload and Enable
echo "Reloading systemd daemon..."
systemctl --user daemon-reload

echo "Enabling and starting timer..."
systemctl --user enable --now ytnova-autofetch.timer

echo "Checking status..."
systemctl --user list-timers --all | grep ytnova

echo ">>> Setup Complete! Git fetch will run every 5 minutes in background."
