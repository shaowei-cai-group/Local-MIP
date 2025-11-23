#!/bin/bash
set -euo pipefail

# DisMIP Build Script
# Usage: ./build.sh [debug|release|clean]

BUILD_TYPE=${1:-release}
BUILD_DIR="build"

case $BUILD_TYPE in
    debug)
        echo "Building Debug version..."
        mkdir -p $BUILD_DIR
        cd $BUILD_DIR
        cmake -DCMAKE_BUILD_TYPE=Debug ..
        make -j$(nproc)
        ;;
    release)
        echo "Building Release version..."
        mkdir -p $BUILD_DIR
        cd $BUILD_DIR
        cmake -DCMAKE_BUILD_TYPE=Release ..
        make -j$(nproc)
        ;;
    clean)
        echo "Cleaning build files..."
        rm -rf $BUILD_DIR
        ;;
    all)
        echo "Building all components (core + examples + python bindings)..."
        
        # 1. Build core library (release mode)
        echo ""
        echo "==> Step 1/4: Building core library (release)..."
        mkdir -p $BUILD_DIR
        cd $BUILD_DIR
        cmake -DCMAKE_BUILD_TYPE=Release ..
        make -j$(nproc)
        cd ..
        
        # 2. Prepare example dependencies
        echo ""
        echo "==> Step 2/4: Preparing example dependencies..."
        cd example
        ./prepare.sh
        
        # 3. Build examples
        echo ""
        echo "==> Step 3/4: Building examples..."
        ./build.sh
        cd ..
        
        # 4. Build python bindings
        echo ""
        echo "==> Step 4/4: Building python bindings..."
        cd python-bindings
        ./build.sh
        cd ..
        
        echo ""
        echo "========================================"
        echo "All components built successfully!"
        echo "========================================"
        echo ""
        echo "Built components:"
        echo "  - Core solver: build/Local-MIP"
        echo "  - Examples: example/*/​*_demo"
        echo "  - Python bindings: python-bindings/build/localmip_py*.so"
        echo ""
        ;;
    *)
        echo "Usage: $0 [debug|release|clean|all]"
        echo "  debug   - Build Debug version (with debug info)"
        echo "  release - Build Release version (optimized)"
        echo "  clean   - Clean build files"
        echo "  all     - Build all components (core + examples + python)"
        exit 1
        ;;
esac
