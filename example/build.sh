#!/bin/bash
#=====================================================================================
# build.sh - Build Local-MIP example programs
#
# Usage:
#   ./build.sh         - Build all examples
#   ./build.sh clean   - Clean build files
#=====================================================================================

set -e  # Exit immediately on error

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Color definitions
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Print colored messages
print_green() {
    echo -e "${GREEN}$1${NC}"
}

print_yellow() {
    echo -e "${YELLOW}$1${NC}"
}

print_red() {
    echo -e "${RED}$1${NC}"
}

# Clean function
clean_build() {
    print_yellow "Cleaning build files..."
    cd "$SCRIPT_DIR"

    # Clean build directory
    if [ -d "build" ]; then
        rm -rf build
        print_green "Deleted build/ directory"
    fi

    # Clean executables in example directories
    for demo in simple-api start-callback restart-callback weight-callback \
                scoring-lift scoring-neighbor neighbor-config neighbor-userdata; do
        if [ -d "$demo" ]; then
            find "$demo" -type f -executable -name "*_demo" -delete
            print_green "Cleaned $demo/ directory"
        fi
    done

    print_green "Cleanup complete!"
    exit 0
}

# Check parameters
if [ "$1" = "clean" ]; then
    clean_build
fi

# Check dependency files
print_yellow "Checking dependencies..."

if [ ! -f "$SCRIPT_DIR/lib/libLocalMIP.a" ]; then
    print_red "Error: lib/libLocalMIP.a not found"
    print_yellow "Please run first: ./prepare.sh"
    exit 1
fi

if [ ! -d "$SCRIPT_DIR/include" ]; then
    print_red "Error: include/ directory not found"
    print_yellow "Please run first: ./prepare.sh"
    exit 1
fi

print_green "Dependency check passed!"

# Create and enter build directory
print_yellow "Creating build directory..."
mkdir -p "$SCRIPT_DIR/build"
cd "$SCRIPT_DIR/build"

# Run CMake
print_yellow "Configuring project..."
cmake .. || {
    print_red "CMake configuration failed!"
    exit 1
}

# Compile
print_yellow "Compiling example programs..."
make -j$(nproc) || {
    print_red "Compilation failed!"
    exit 1
}

print_green ""
print_green "========================================"
print_green "Compilation successful!"
print_green "========================================"
print_green ""
print_green "Executable file list:"
echo ""

# List all generated executables
demos=(
    "simple-api/simple_api_demo"
    "start-callback/start_callback_demo"
    "restart-callback/restart_callback_demo"
    "weight-callback/weight_callback_demo"
    "scoring-lift/lift_degree_demo"
    "scoring-neighbor/neighbor_random_demo"
    "neighbor-config/neighbor_config_demo"
    "neighbor-userdata/neighbor_userdata_demo"
)

for demo in "${demos[@]}"; do
    if [ -f "$SCRIPT_DIR/$demo" ]; then
        echo "  ./$demo"
    fi
done

echo ""
print_green "Run example:"
echo "  cd $SCRIPT_DIR"
echo "  ./simple-api/simple_api_demo"
echo ""
