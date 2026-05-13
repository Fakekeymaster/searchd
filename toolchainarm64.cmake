# toolchain-arm64.cmake
# Cross-compilation toolchain for ARM64 (aarch64) targets.
# This mirrors the sysroot pipeline used for embedded switch targets.
#
# Usage:
#   cmake .. -DCMAKE_TOOLCHAIN_FILE=../toolchain-arm64.cmake

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

# Path to the cross-compiler — install with:
#   sudo apt install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu
set(CMAKE_C_COMPILER   aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)

# Sysroot: the root filesystem of the target device.
# Set this to your actual sysroot path if you have one.
# set(CMAKE_SYSROOT /path/to/arm64/sysroot)

# Search paths: look in the sysroot first, then the host
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
