cmake_minimum_required(VERSION 3.13)
project(sphcode CXX)

# Conan integration
if(EXISTS ${CMAKE_BINARY_DIR}/conan_toolchain.cmake)
  include(${CMAKE_BINARY_DIR}/conan_toolchain.cmake)
endif()

# Make OpenMP optional for compatibility with different environments
find_package(OpenMP)
if(OpenMP_CXX_FOUND)
  message(STATUS "OpenMP found - enabling parallel support")
else()
  message(WARNING "OpenMP not found - building without parallel support")
endif()

find_package(Boost REQUIRED)
find_package(nlohmann_json REQUIRED)

# Create library for core SPH functionality (used by both executable and tests)
add_library(sph_lib STATIC)
target_compile_features(sph_lib PUBLIC cxx_std_14)
target_compile_options(sph_lib
  PUBLIC
    -Wall
    -Wno-sign-compare
    -Wno-maybe-uninitialized
    -funroll-loops
    -ffast-math
  )
target_include_directories(sph_lib PUBLIC include)
target_link_libraries(sph_lib
  PUBLIC
    Boost::boost
    nlohmann_json::nlohmann_json
  )

# Link OpenMP only if found
if(OpenMP_CXX_FOUND)
  target_link_libraries(sph_lib PUBLIC OpenMP::OpenMP_CXX)
  target_compile_definitions(sph_lib PUBLIC SPH_USE_OPENMP)
endif()

# Create executable
add_executable(sph)
target_link_libraries(sph PRIVATE sph_lib)

add_subdirectory(include)
add_subdirectory(src)

# Testing
option(BUILD_TESTING "Build tests" ON)
if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
