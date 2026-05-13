cmake_minimum_required(VERSION 3.16)
project(searchd VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Build type ────────────────────────────────────────────────────────────────
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# ── Compiler flags ────────────────────────────────────────────────────────────
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG   "-g -O0 -fsanitize=address,undefined")

# ── Sources ───────────────────────────────────────────────────────────────────
set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../searcher.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../dir_walker.cpp
)

add_executable(searchd ${SOURCES})

# ── Include path (headers are in project root) ───────────────────────────────
target_include_directories(searchd PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

# ── Link filesystem and threads ───────────────────────────────────────────────
# std::filesystem needs -lstdc++fs on older GCC (GCC < 9)
# On macOS/Apple Clang, it's built into libc++ — no extra link needed
find_package(Threads REQUIRED)
if(APPLE)
    target_link_libraries(searchd PRIVATE Threads::Threads)
else()
    target_link_libraries(searchd PRIVATE Threads::Threads stdc++fs)
endif()

# ── Install ───────────────────────────────────────────────────────────────────
install(TARGETS searchd DESTINATION bin)

# ── ARM64 cross-compilation notes ────────────────────────────────────────────
# To cross-compile for ARM64 (same pipeline used in production at Tejas Networks):
#
#   cmake .. \
#     -DCMAKE_TOOLCHAIN_FILE=../toolchain-arm64.cmake \
#     -DCMAKE_BUILD_TYPE=Release
#
# See toolchain-arm64.cmake for the sysroot configuration.
