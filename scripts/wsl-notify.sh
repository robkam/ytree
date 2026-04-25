#!/usr/bin/env bash
# WSL2 to Windows Notification Bridge
# Uses powershell.exe to send a Toast notification.

TITLE="${1:-Antigravity}"
MESSAGE="${2:-Task completed}"

# Use PowerShell to trigger a simple system notification
# This works out of the box on Windows 10/11 through WSL2
powershell.exe -NoProfile -Command "
    \$t = '${TITLE}';
    \$m = '${MESSAGE}';
    [reflection.assembly]::loadwithpartialname('System.Windows.Forms');
    \$n = New-Object System.Windows.Forms.NotifyIcon;
    \$n.Icon = [System.Drawing.SystemIcons]::Information;
    \$n.Visible = \$true;
    \$n.ShowBalloonTip(5000, \$t, \$m, [System.Windows.Forms.ToolTipIcon]::Info);
    Start-Sleep -Seconds 1;
    \$n.Dispose();
" > /dev/null 2>&1
