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

# FFI library for Rust benchmarks (static library)
add_library(limcode_benchmark_ffi STATIC src/limcode_benchmark_ffi.cpp)
target_include_directories(limcode_benchmark_ffi PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)
target_link_libraries(limcode_benchmark_ffi PUBLIC limcode)

# Solana snapshot parser library (requires libarchive)
find_package(PkgConfig)
if(PkgConfig_FOUND)
  pkg_check_modules(LIBARCHIVE libarchive)
  if(LIBARCHIVE_FOUND)
    add_library(limcode_snapshot STATIC src/snapshot.cpp)
    target_include_directories(limcode_snapshot PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:include>
      ${LIBARCHIVE_INCLUDE_DIRS}
    )
    target_link_libraries(limcode_snapshot PUBLIC limcode ${LIBARCHIVE_LIBRARIES})
    target_compile_options(limcode_snapshot PRIVATE ${LIBARCHIVE_CFLAGS_OTHER})
    message(STATUS "Solana snapshot support enabled (libarchive found)")
  else()
    message(STATUS "Solana snapshot support disabled (libarchive not found)")
  endif()
endif()

# Performance benchmark
add_executable(bench benchmark/bench.cpp)
target_link_libraries(bench PRIVATE limcode)

# Pure C++ benchmark (theoretical maximum performance)
add_executable(benchmark_pure_cpp benchmark/benchmark_pure_cpp.cpp)
target_link_libraries(benchmark_pure_cpp PRIVATE limcode)

# Theoretical maximum benchmark (AVX-512 16x unrolling + OpenMP)
add_executable(bench_theoretical_max benchmark/bench_theoretical_max.cpp)
target_link_libraries(bench_theoretical_max PRIVATE limcode)
find_package(OpenMP)
if(OpenMP_CXX_FOUND)
  target_link_libraries(bench_theoretical_max PRIVATE OpenMP::OpenMP_CXX)
endif()

# True theoretical maximum (optimized with NT stores, prefetch, RDTSC)
add_executable(bench_true_maximum benchmark/bench_true_maximum.cpp)
target_link_libraries(bench_true_maximum PRIVATE limcode)

# C++ Snapshot parser test (requires libarchive)
if(LIBARCHIVE_FOUND)
  add_executable(cpp_snapshot_test benchmark/cpp_snapshot_test.cpp)
  target_link_libraries(cpp_snapshot_test PRIVATE limcode_snapshot)
endif()

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
