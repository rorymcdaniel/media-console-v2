#!/usr/bin/env bash
set -euo pipefail

# Media Console Kiosk Install Script
# Run as root: sudo ./install-kiosk.sh

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SERVICE_FILE="$SCRIPT_DIR/media-console.service"
BINARY="/home/rory/Code/media-console-v2/build/media-console"
USER="rory"

echo "=== Media Console Kiosk Install ==="

# Check prerequisites
if [[ $EUID -ne 0 ]]; then
    echo "Error: Run with sudo"
    exit 1
fi

if [[ ! -f "$BINARY" ]]; then
    echo "Error: $BINARY not found. Run pi-build.sh first."
    exit 1
fi

# 1. Install systemd service (PROD-01)
echo "Installing systemd service..."
cp "$SERVICE_FILE" /etc/systemd/system/media-console.service
systemctl daemon-reload
systemctl enable media-console.service
echo "  Done: Service installed and enabled"

# 2. Configure auto-login on tty1 (PROD-02)
echo "Configuring auto-login..."
mkdir -p /etc/systemd/system/getty@tty1.service.d
cat > /etc/systemd/system/getty@tty1.service.d/autologin.conf << EOF
[Service]
ExecStart=
ExecStart=-/sbin/agetty --autologin $USER --noclear %I \$TERM
EOF
echo "  Done: Auto-login configured for $USER on tty1"

# 3. Install unclutter for hidden cursor (PROD-02)
echo "Installing unclutter..."
if ! command -v unclutter &>/dev/null; then
    apt-get install -y unclutter
fi

# Add unclutter to user's .bashrc for kiosk sessions
PROFILE_DIR="/home/$USER"
if ! grep -q "unclutter" "$PROFILE_DIR/.bashrc" 2>/dev/null; then
    cat >> "$PROFILE_DIR/.bashrc" << 'EOF'

# Hide cursor for kiosk mode (media-console)
if [[ -z "$SSH_CONNECTION" ]] && command -v unclutter &>/dev/null; then
    unclutter -idle 0.5 -root &
fi
EOF
    chown "$USER:$USER" "$PROFILE_DIR/.bashrc"
fi
echo "  Done: Cursor hiding configured"

# 4. Disable screen blanking (PROD-02)
echo "Disabling screen blanking..."
# For console blanking
if ! grep -q "consoleblank=0" /boot/firmware/cmdline.txt 2>/dev/null; then
    sed -i 's/$/ consoleblank=0/' /boot/firmware/cmdline.txt
fi
echo "  Done: Screen blanking disabled"

# 5. Create default config if not exists
CONFIG_DIR="/home/$USER/.config/MediaConsole"
if [[ ! -f "$CONFIG_DIR/media-console.conf" ]]; then
    mkdir -p "$CONFIG_DIR"
    cat > "$CONFIG_DIR/media-console.conf" << EOF
[General]
version=2.0.0

[receiver]
host=192.168.1.100
port=60128

[audio]
device=hw:2,0

[display]
dim_timeout_sec=300
off_timeout_sec=1200
dim_brightness=25

[api]
port=8080

[library]
root=/media/flac
EOF
    chown -R "$USER:$USER" "$CONFIG_DIR"
    echo "  Done: Default config created at $CONFIG_DIR/media-console.conf"
fi

echo ""
echo "=== Installation Complete ==="
echo ""
echo "Start now:  sudo systemctl start media-console"
echo "View logs:  journalctl -u media-console -f"
echo "Status:     sudo systemctl status media-console"
echo ""
echo "The service will start automatically on next boot."
