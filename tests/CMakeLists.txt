cmake_minimum_required(VERSION 3.13)

# Use FetchContent to get Google Test
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.14.0
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

include(GoogleTest)

# Enable testing
enable_testing()

# Test executable
add_executable(sph_tests
    test_main.cpp
)

target_include_directories(sph_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(sph_tests
    PRIVATE
        sph_lib
        GTest::gtest
        GTest::gtest_main
        OpenMP::OpenMP_CXX
        Boost::boost
        nlohmann_json::nlohmann_json
)

target_compile_features(sph_tests PUBLIC cxx_std_17)
target_compile_options(sph_tests
    PRIVATE
        -Wall
        -Wno-sign-compare
        -Wno-maybe-uninitialized
)

# Tests use Dim=1 for simplicity (defined via constexpr in each test file)
# No DIM macro needed

# Add subdirectories for three-tier test structure
# Unit tests - fast, isolated component tests
add_subdirectory(unit/core/particles)
add_subdirectory(unit/core/neighbors)
add_subdirectory(unit/core/kernels)
add_subdirectory(unit/core/parameters)
add_subdirectory(unit/core/output)
add_subdirectory(unit/core/dimension)
add_subdirectory(unit/core/units)
add_subdirectory(unit/algorithms/riemann)
add_subdirectory(unit/algorithms/limiters)
add_subdirectory(unit/algorithms/sph)
add_subdirectory(unit/algorithms/viscosity)
add_subdirectory(unit/tree)
add_subdirectory(unit/boundary)
add_subdirectory(unit/utilities)

# Integration tests - component interaction tests
add_subdirectory(integration/ghost_boundary)
add_subdirectory(integration/neighbor_search)
add_subdirectory(integration/coordinators)
add_subdirectory(integration/output_pipeline)
add_subdirectory(integration/plugins)

# Keep legacy directories for now (will be removed in Phase 4)
add_subdirectory(modules)
add_subdirectory(numerical)
add_subdirectory(performance)
add_subdirectory(regression)

# Discover tests
gtest_discover_tests(sph_tests)

# Custom test target
add_custom_target(run_tests
    COMMAND sph_tests --gtest_color=yes
    DEPENDS sph_tests
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
