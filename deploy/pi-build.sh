#!/usr/bin/env bash
set -euo pipefail

# Build script for media-console-v2 on Raspberry Pi.
# Incremental: cmake cache and build artifacts are preserved between runs.
# Binary output: build/media-console (relative to repo root)

REPO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$REPO_DIR/build"
TIMESTAMP() { date '+%Y-%m-%d %H:%M:%S'; }

echo "[$(TIMESTAMP)] === media-console-v2 build ==="
echo "[$(TIMESTAMP)] Repo:      $REPO_DIR"
echo "[$(TIMESTAMP)] Build dir: $BUILD_DIR"
echo "[$(TIMESTAMP)] CMake:     $(cmake --version | head -1)"
echo "[$(TIMESTAMP)] Ninja:     $(ninja --version)"

echo "[$(TIMESTAMP)] Configuring..."
cmake -B "$BUILD_DIR" \
  -DQt6_DIR=/opt/Qt/6.9.2/gcc_arm64/lib/cmake/Qt6 \
  -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTS=OFF

echo "[$(TIMESTAMP)] Building..."
cmake --build "$BUILD_DIR"

echo "[$(TIMESTAMP)] === Build complete ==="
echo "[$(TIMESTAMP)] Binary: $BUILD_DIR/media-console"

echo "[$(TIMESTAMP)] Restarting media-console service..."
sudo systemctl restart media-console

echo "[$(TIMESTAMP)] Waiting 3 seconds for service to start..."
sleep 3

echo "[$(TIMESTAMP)] Checking service status..."
sudo systemctl status media-console --no-pager || true
