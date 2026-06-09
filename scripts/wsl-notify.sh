#!/usr/bin/env bash
# WSL2 to Windows Notification Bridge

TITLE="${1:-ytree}"
MESSAGE="${2:-Task completed}"

powershell_script=$(cat <<'POWERSHELL'
$ErrorActionPreference = 'Stop'

$title = $env:WSL_NOTIFY_TITLE
$message = $env:WSL_NOTIFY_MESSAGE
$appId = 'ytree.codex.notifications'

function Show-WinRtToast {
    param(
        [string] $Title,
        [string] $Message,
        [string] $AppId
    )

    try {
        [Windows.UI.Notifications.ToastNotificationManager, Windows.UI.Notifications, ContentType = WindowsRuntime] | Out-Null
        [Windows.UI.Notifications.ToastNotification, Windows.UI.Notifications, ContentType = WindowsRuntime] | Out-Null
        [Windows.Data.Xml.Dom.XmlDocument, Windows.Data.Xml.Dom.XmlDocument, ContentType = WindowsRuntime] | Out-Null

        $xml = [Windows.UI.Notifications.ToastNotificationManager]::GetTemplateContent(
            [Windows.UI.Notifications.ToastTemplateType]::ToastText02
        )
        $textNodes = $xml.GetElementsByTagName('text')
        $textNodes.Item(0).AppendChild($xml.CreateTextNode($Title)) | Out-Null
        $textNodes.Item(1).AppendChild($xml.CreateTextNode($Message)) | Out-Null

        $toast = [Windows.UI.Notifications.ToastNotification]::new($xml)
        $toast.Group = 'ytree'
        $toast.Tag = 'codex-notification'

        [Windows.UI.Notifications.ToastNotificationManager]::CreateToastNotifier($AppId).Show($toast)
        return $true
    } catch {
        return $false
    }
}

function Show-LegacyBalloon {
    param(
        [string] $Title,
        [string] $Message
    )

    try {
        Add-Type -AssemblyName System.Windows.Forms
        Add-Type -AssemblyName System.Drawing

        $notification = New-Object System.Windows.Forms.NotifyIcon
        $notification.Icon = [System.Drawing.SystemIcons]::Information
        $notification.Visible = $true
        $notification.ShowBalloonTip(5000, $Title, $Message, [System.Windows.Forms.ToolTipIcon]::Info)
        Start-Sleep -Seconds 1
        $notification.Dispose()
        return $true
    } catch {
        return $false
    }
}

if (Show-WinRtToast -Title $title -Message $message -AppId $appId) {
    exit 0
}

if (Show-LegacyBalloon -Title $title -Message $message) {
    exit 0
}

exit 1
POWERSHELL
)

encoded_script=$(printf '%s' "$powershell_script" | iconv -f UTF-8 -t UTF-16LE | base64 | tr -d '\n')

WSL_NOTIFY_TITLE="$TITLE" WSL_NOTIFY_MESSAGE="$MESSAGE" \
    powershell.exe -NoProfile -ExecutionPolicy Bypass -EncodedCommand "$encoded_script" > /dev/null 2>&1
