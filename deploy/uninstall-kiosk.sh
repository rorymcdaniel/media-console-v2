#!/usr/bin/env bash
set -euo pipefail

# Media Console Kiosk Uninstall Script
# Run as root: sudo ./uninstall-kiosk.sh

echo "=== Media Console Kiosk Uninstall ==="

if [[ $EUID -ne 0 ]]; then
    echo "Error: Run with sudo"
    exit 1
fi

# 1. Stop and disable service
echo "Stopping service..."
systemctl stop media-console.service 2>/dev/null || true
systemctl disable media-console.service 2>/dev/null || true
rm -f /etc/systemd/system/media-console.service
systemctl daemon-reload
echo "  Done: Service removed"

# 2. Remove auto-login
echo "Removing auto-login..."
rm -f /etc/systemd/system/getty@tty1.service.d/autologin.conf
rmdir /etc/systemd/system/getty@tty1.service.d 2>/dev/null || true
echo "  Done: Auto-login removed"

# 3. Note: Not removing unclutter or screen blanking changes
#    (they may be wanted for other purposes)

echo ""
echo "=== Uninstall Complete ==="
echo ""
echo "Note: unclutter and screen blanking settings were not changed."
echo "Config files preserved at ~/.config/MediaConsole/"
echo ""
