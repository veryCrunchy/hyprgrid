#!/bin/bash

# Build script for Hypr Grid Manager
# This script builds the Qt6 Grid Manager for Hyprland

set -e

# Configuration
BUILD_DIR="build"
INSTALL_PREFIX="/usr/local"
BUILD_TYPE="Release"

# Parse arguments
while [[ $# -gt 0 ]]; do
  key="$1"
  case $key in
    --debug)
      BUILD_TYPE="Debug"
      shift
      ;;
    --install)
      DO_INSTALL=1
      shift
      ;;
    --clean)
      CLEAN_BUILD=1
      shift
      ;;
    --prefix)
      INSTALL_PREFIX="$2"
      shift
      shift
      ;;
    *)
      echo "Unknown option: $1"
      echo "Usage: $0 [--debug] [--install] [--clean] [--prefix <path>]"
      exit 1
      ;;
  esac
done

# Print build configuration
echo "Building Hypr Grid Manager with configuration:"
echo "  Build type: $BUILD_TYPE"
echo "  Install prefix: $INSTALL_PREFIX"
echo "  Clean build: ${CLEAN_BUILD:-0}"
echo "  Install after build: ${DO_INSTALL:-0}"
echo

# Clean build directory if requested
if [ -n "$CLEAN_BUILD" ] && [ -d "$BUILD_DIR" ]; then
  echo "Cleaning build directory..."
  rm -rf "$BUILD_DIR"
fi

# Create build directory if it doesn't exist
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Run CMake
echo "Configuring with CMake..."
cmake .. \
  -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
  -DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX"

# Build the project
echo "Building the project..."
cmake --build . -- -j$(nproc)

# Install if requested
if [ -n "$DO_INSTALL" ]; then
  echo "Installing to $INSTALL_PREFIX..."
  sudo cmake --install .
fi

echo "Build completed successfully!"

# Return to the original directory
cd ..
