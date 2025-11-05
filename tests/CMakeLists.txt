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

# Define dimension for tests
target_compile_definitions(sph_tests PRIVATE DIM=1)

# Add subdirectories for organized tests
add_subdirectory(core)
add_subdirectory(algorithms)
add_subdirectory(tree)
add_subdirectory(modules)
add_subdirectory(utilities)
add_subdirectory(boundary)
add_subdirectory(integration)
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
