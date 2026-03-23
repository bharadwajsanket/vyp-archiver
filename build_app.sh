#!/bin/bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
cd "$ROOT"

echo "=== Step 1: Clean old build artifacts ==="
make clean 2>/dev/null || true
rm -rf "VYP Archiver.app"
rm -rf gui/build

echo "=== Step 2: Build engine (vypack) ==="
make -j$(sysctl -n hw.ncpu)
echo "Engine built: $(file vypack)"

echo "=== Step 3: Build GUI ==="
mkdir -p gui/build
cd gui/build
qmake6 ../vyp_gui.pro -spec macx-clang CONFIG+=release
make -j$(sysctl -n hw.ncpu)
cd "$ROOT"

echo "=== Step 4: Assemble VYP Archiver.app ==="

# Find the .app produced by qmake
APP_SRC=""
for candidate in "gui/build/VYP Archiver.app" "gui/build/VYP_Archiver.app"; do
    if [ -d "$candidate" ]; then
        APP_SRC="$candidate"
        break
    fi
done

if [ -n "$APP_SRC" ]; then
    cp -R "$APP_SRC" "VYP Archiver.app"
else
    echo "ERROR: qmake did not produce an .app bundle"
    ls -la gui/build/
    exit 1
fi

# ── CRITICAL: Copy vypack engine into the app bundle ──
MACOS_DIR="VYP Archiver.app/Contents/MacOS"
cp -f "$ROOT/vypack" "$MACOS_DIR/vypack"
chmod +x "$MACOS_DIR/vypack"
chmod +x "$MACOS_DIR/VYP Archiver" 2>/dev/null || true

echo "Engine copied into bundle: $MACOS_DIR/vypack"

# ── Bundle Qt frameworks ──
if command -v macdeployqt6 &>/dev/null; then
    echo "Running macdeployqt6..."
    macdeployqt6 "VYP Archiver.app" 2>/dev/null || true
elif command -v macdeployqt &>/dev/null; then
    echo "Running macdeployqt..."
    macdeployqt "VYP Archiver.app" 2>/dev/null || true
else
    echo "WARNING: macdeployqt not found — Qt frameworks may not be bundled."
fi

# ── CRITICAL: Re-sign the entire bundle after macdeployqt ──
# macdeployqt modifies binaries (rpaths, framework embedding) which
# invalidates code signatures. Without re-signing, macOS kills the app
# on launch with EXC_BAD_ACCESS (Code Signature Invalid).
echo "Codesigning app bundle..."
codesign --force --deep --sign - "VYP Archiver.app"
echo "Codesigning complete."

echo ""
echo "=== Build Complete ==="
echo ""
echo "App bundle: $ROOT/VYP Archiver.app"
echo ""
echo "Contents of MacOS/:"
ls -la "$MACOS_DIR/"
echo ""

# ── Final verification ──
if [ -f "$MACOS_DIR/VYP Archiver" ] && [ -f "$MACOS_DIR/vypack" ]; then
    echo "✅ Both binaries present — app is ready."
    echo "   open \"$ROOT/VYP Archiver.app\""
else
    echo "❌ ERROR: Missing binaries!"
    exit 1
fi
