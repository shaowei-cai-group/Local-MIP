#!/bin/bash
#=====================================================================================
# prepare.sh - Prepare independent dependency files for example directory
#
# This script copies necessary files from main project to example directory
# to enable independent compilation and execution
#=====================================================================================

set -e  # Exit immediately on error

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

echo "========================================"
echo "Preparing example directory dependencies"
echo "========================================"

# 1. Ensure main project has been built
if [ ! -f "$PROJECT_ROOT/build/libLocalMIP.a" ]; then
    echo "Error: libLocalMIP.a not found"
    echo "Please run in main directory first: ./build.sh release"
    exit 1
fi

# 2. Create necessary directories
echo ""
echo "Creating directory structure..."
mkdir -p "$SCRIPT_DIR/lib"
mkdir -p "$SCRIPT_DIR/include"
mkdir -p "$SCRIPT_DIR/test-set"

# 3. Copy static library
echo ""
echo "Copying static library..."
cp -v "$PROJECT_ROOT/build/libLocalMIP.a" "$SCRIPT_DIR/lib/"

# 4. Copy header files (preserve directory structure)
echo ""
echo "Copying header files..."
cd "$PROJECT_ROOT/src"
find . -name "*.h" | while read header; do
    target_dir="$SCRIPT_DIR/include/$(dirname "$header")"
    mkdir -p "$target_dir"
    cp -v "$header" "$target_dir/"
done

# 5. Copy test instances
echo ""
echo "Copying test instances..."
if [ -d "$PROJECT_ROOT/test-set" ]; then
    # Copy representative test instances
    test_instances=(
        "2club200v15p5scn.mps"
        "sct1.mps"
        "allcolor10.mps"
        "22433.mps"
        "30n20b8.mps"
    )

    for instance in "${test_instances[@]}"; do
        if [ -f "$PROJECT_ROOT/test-set/$instance" ]; then
            cp -v "$PROJECT_ROOT/test-set/$instance" "$SCRIPT_DIR/test-set/"
        fi
    done
fi

echo ""
echo "========================================"
echo "Preparation complete!"
echo "========================================"
echo ""
echo "File list:"
echo "  lib/libLocalMIP.a         - Static library"
echo "  include/                  - Header file directory"
echo "  test-set/                 - Test instances"
echo ""
echo "Next steps:"
echo "  ./build.sh                - Compile all examples"
echo "  ./simple-api/simple_api_demo  - Run example"
echo ""
