#!/bin/bash

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
    *)
        echo "Usage: $0 [debug|release|clean]"
        echo "  debug   - Build Debug version (with debug info)"
        echo "  release - Build Release version (optimized)"
        echo "  clean   - Clean build files"
        exit 1
        ;;
esac
