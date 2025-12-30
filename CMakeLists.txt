cmake_minimum_required(VERSION 3.16)
project(limcode VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Compiler flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -march=native -flto")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
endif()

# Header-only library
add_library(limcode INTERFACE)
target_include_directories(limcode INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

# Benchmark executable
add_executable(limcode_benchmark benchmark/main.cpp)
target_link_libraries(limcode_benchmark PRIVATE limcode)

# Tests
enable_testing()
add_executable(limcode_tests tests/test_limcode.cpp)
target_link_libraries(limcode_tests PRIVATE limcode)
add_test(NAME limcode_tests COMMAND limcode_tests)

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
