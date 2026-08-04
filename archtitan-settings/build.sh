#!/usr/bin/env bash
# ArchTitan Settings — Quick Build Script
set -e

PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$PROJECT_DIR/build"

echo "╔══════════════════════════════════════╗"
echo "║  ArchTitan Settings — Build          ║"
echo "╚══════════════════════════════════════╝"
echo ""

# Ensure build dir exists
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure
echo "→ Configuring with CMake..."
cmake .. \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=/usr

# Build
echo "→ Building..."
make -j$(nproc)

echo ""
echo "✓ Build complete!"
echo "  Binary: $BUILD_DIR/archtitan-settings"
echo ""
echo "  Run with: $BUILD_DIR/archtitan-settings"
