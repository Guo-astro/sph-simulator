cmake_minimum_required(VERSION 3.13)
project(sphcode CXX)

# Conan integration
if(EXISTS ${CMAKE_BINARY_DIR}/conan_toolchain.cmake)
  include(${CMAKE_BINARY_DIR}/conan_toolchain.cmake)
endif()

find_package(OpenMP REQUIRED)
find_package(Boost REQUIRED)
find_package(nlohmann_json REQUIRED)

add_executable(sph)
target_compile_features(sph PUBLIC cxx_std_14)
target_compile_options(sph
  PUBLIC
    -Wall
    -Wno-sign-compare
    -Wno-maybe-uninitialized
    -funroll-loops
    -ffast-math
  )
target_include_directories(sph PUBLIC include)
target_link_libraries(sph
  PUBLIC
    OpenMP::OpenMP_CXX
    Boost::boost
    nlohmann_json::nlohmann_json
  )

add_subdirectory(include)
add_subdirectory(src)
