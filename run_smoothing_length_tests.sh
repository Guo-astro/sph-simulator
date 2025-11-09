#!/usr/bin/env bash
# Standalone test runner for smoothing length configuration tests

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TEST_FILE="$SCRIPT_DIR/tests/unit/core/parameters/smoothing_length_config_test.cpp"
BUILD_DIR="$SCRIPT_DIR/build_standalone_test"
mkdir -p "$BUILD_DIR"

echo "==> Building smoothing length configuration tests..."

# Compile test
g++ -std=c++17 -I"$SCRIPT_DIR/include" \
    -I"$SCRIPT_DIR/build/_deps/googletest-src/googletest/include" \
    -L"$SCRIPT_DIR/build/lib" \
    "$TEST_FILE" \
    "$SCRIPT_DIR/build/lib/libgtest.a" \
    "$SCRIPT_DIR/build/lib/libgtest_main.a" \
    "$SCRIPT_DIR/build/lib/libsph_lib.a" \
    -pthread \
    -o "$BUILD_DIR/smoothing_length_test" \
    2>&1 || {
        echo "==> Compilation failed. Trying with CMake object files..."
        # The test needs the full library to be built
        cd "$SCRIPT_DIR/build" && make sph_lib -j8
        exit 1
    }

echo "==> Running tests..."
"$BUILD_DIR/smoothing_length_test" --gtest_color=yes

echo "==> ✓ All smoothing length configuration tests passed!"
