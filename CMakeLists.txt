cmake_minimum_required(VERSION 3.16)
project(limcode VERSION 0.1.2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Hyper-optimization for maximum speed
option(ENABLE_HYPER_OPTIMIZATION "Enable aggressive optimizations for speed and memory efficiency" ON)
option(ENABLE_PGO_GENERATE "Generate PGO profile data" OFF)
option(ENABLE_PGO_USE "Use PGO profile data for optimization" OFF)

if(ENABLE_HYPER_OPTIMIZATION AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  # In CI, use conservative baseline to avoid SIGILL on different runners
  # In local builds, use -march=native for maximum performance
  if(DEFINED ENV{CI} OR DEFINED ENV{GITHUB_ACTIONS})
    set(MARCH_FLAG "-march=x86-64-v2")
    message(STATUS "CI detected: Using -march=x86-64-v2 for compatibility")
  else()
    set(MARCH_FLAG "-march=native -mtune=native")
    message(STATUS "Hyper-optimization enabled: -O3 -march=native -flto -ffast-math")
  endif()

  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
  set(CMAKE_CXX_FLAGS_RELEASE "-O3 ${MARCH_FLAG} -flto -DNDEBUG")
  set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")

  # ULTRA-AGGRESSIVE optimizations for maximum speed
  add_compile_options(
    # Math optimizations
    -ffast-math                 # Aggressive FP math (unsafe but fast)
    -fno-math-errno            # Don't set errno for math functions
    -ffinite-math-only         # Assume no NaN/Inf
    -fno-signed-zeros          # Ignore sign of zero
    -fno-trapping-math         # No FP exceptions
    -fassociative-math         # Reassociate FP operations

    # Loop optimizations
    -funroll-loops              # Unroll loops aggressively
    -funroll-all-loops          # Unroll ALL loops
    -fvariable-expansion-in-unroller  # Better unrolling
    -fpeel-loops               # Peel loop iterations
    -funswitch-loops           # Loop unswitching

    # Inlining
    -finline-functions         # Inline everything possible
    -finline-limit=1000        # Huge inline limit

    # Code generation
    -fno-omit-frame-pointer    # Better profiling
    -fno-plt                   # No PLT for faster calls
    -fno-stack-protector       # Remove stack canaries (unsafe but fast)

    # Aliasing
    -fstrict-aliasing          # Strict aliasing rules
    -fno-semantic-interposition  # No symbol interposition

    # SIMD
    -mavx512f -mavx512bw -mavx512dq -mavx512vl  # All AVX-512
    -mavx2 -msse4.2            # AVX2/SSE4.2 fallback
    -mbmi2                     # PDEP/PEXT for varint
    -mprefer-vector-width=512  # Prefer 512-bit vectors

    # Branch prediction
    -fbranch-probabilities     # Better branch prediction
  )

  # LTO for whole-program optimization
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

  # Additional linker optimizations
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -flto=auto -fuse-linker-plugin")

  # Profile-Guided Optimization (PGO)
  if(ENABLE_PGO_GENERATE)
    message(STATUS "PGO: Generating profile data (-fprofile-generate)")
    add_compile_options(-fprofile-generate)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fprofile-generate")
  elseif(ENABLE_PGO_USE)
    message(STATUS "PGO: Using profile data (-fprofile-use)")
    add_compile_options(-fprofile-use -fprofile-correction)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fprofile-use")
  endif()
endif()

# Header-only library
add_library(limcode INTERFACE)
target_include_directories(limcode INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

# FFI library for Rust bindings (static library)
add_library(limcode_ffi STATIC src/limcode_ffi.cpp)
target_include_directories(limcode_ffi PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)
target_link_libraries(limcode_ffi PUBLIC limcode)

# PGO Profiling Benchmark
add_executable(pgo_profile benchmark/pgo_profile.cpp)
target_link_libraries(pgo_profile PRIVATE limcode)

# Optimized API Benchmark
add_executable(optimized_bench benchmark/optimized_bench.cpp)
target_link_libraries(optimized_bench PRIVATE limcode)

# Standalone C++ Performance Benchmark (no Rust dependency)
add_executable(cpp_standalone_bench benchmark/cpp_standalone_bench.cpp)
target_link_libraries(cpp_standalone_bench PRIVATE limcode)

# Ultra-Fast C++ Benchmark (SIMD, zero-copy, lock-free, multithreaded)
add_executable(ultra_fast_bench benchmark/ultra_fast_bench.cpp)
target_link_libraries(ultra_fast_bench PRIVATE limcode)

# Investigation: 64MB performance
add_executable(investigate_64mb benchmark/investigate_64mb.cpp)
target_link_libraries(investigate_64mb PRIVATE limcode)

# EXTREME mode benchmark (multi-threaded parallel copy - 120+ GiB/s target)
add_executable(extreme_bench benchmark/extreme_bench.cpp)
target_link_libraries(extreme_bench PRIVATE limcode numa pthread)

# INSANE mode benchmark (inline assembly, 16x unrolling, 32 ZMM registers - 20+ GiB/s target)
add_executable(insane_bench benchmark/insane_bench.cpp)
target_link_libraries(insane_bench PRIVATE limcode numa pthread)

# ULTIMATE mode benchmark (32x unrolling, 2048 bytes/iter - 22.39 GiB/s target = 100% HW max)
add_executable(ultimate_bench benchmark/ultimate_bench.cpp)
target_link_libraries(ultimate_bench PRIVATE limcode numa pthread)

# Table format benchmark (for README comparison with Rust)
add_executable(table_bench benchmark/table_bench.cpp)
target_link_libraries(table_bench PRIVATE limcode)

# INSANE MODE table bench (aligned + restrict)
add_executable(table_bench_insane benchmark/table_bench_insane.cpp)
target_link_libraries(table_bench_insane PRIVATE limcode)

# ULTIMATE MODE - AVX-512 16x unrolled (99%+ efficiency)
add_executable(table_bench_ultimate benchmark/table_bench_ultimate.cpp)
target_link_libraries(table_bench_ultimate PRIVATE limcode)

# Overhead analysis - find the 7 GB/s gap
add_executable(overhead_analysis benchmark/overhead_analysis.cpp)
target_link_libraries(overhead_analysis PRIVATE limcode)

# Final overhead analysis - answer "why not 99%?"
add_executable(final_overhead_analysis benchmark/final_overhead_analysis.cpp)
target_link_libraries(final_overhead_analysis PRIVATE limcode)

# Standalone serialize - single test, no interference
add_executable(standalone_serialize benchmark/standalone_serialize.cpp)
target_link_libraries(standalone_serialize PRIVATE limcode)

# UNSAFE maximum speed - fuck safety edition
add_executable(unsafe_maximum_speed benchmark/unsafe_maximum_speed.cpp)
target_link_libraries(unsafe_maximum_speed PRIVATE limcode)

# BEYOND LIMITS - attack the 8% gap
add_executable(beyond_limits benchmark/beyond_limits.cpp)
target_link_libraries(beyond_limits PRIVATE limcode)

# Tests
enable_testing()
add_executable(limcode_tests tests/test_limcode.cpp)
target_link_libraries(limcode_tests PRIVATE limcode)
add_test(NAME limcode_tests COMMAND limcode_tests)

# Bincode compatibility test (standalone C++ - no Rust dependency)
add_executable(cpp_bincode_compat tests/cpp_bincode_compat.cpp)
target_link_libraries(cpp_bincode_compat PRIVATE limcode)
add_test(NAME cpp_bincode_compat COMMAND cpp_bincode_compat)

# Install
include(GNUInstallDirs)
install(TARGETS limcode
  EXPORT limcodeTargets
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(DIRECTORY include/limcode DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT limcodeTargets
  FILE limcodeTargets.cmake
  NAMESPACE limcode::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/limcode
)
