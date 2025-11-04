cmake_minimum_required(VERSION 3.13)
project(sphcode CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Quality tools enabled by default for strict code quality
option(ENABLE_CLANG_TIDY "Enable clang-tidy checks during build" ON)

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

# Setup clang-tidy if enabled
if(ENABLE_CLANG_TIDY)
  # Try common Homebrew paths first
  find_program(CLANG_TIDY_EXECUTABLE 
    NAMES clang-tidy
    PATHS 
      /opt/homebrew/opt/llvm/bin
      /usr/local/opt/llvm/bin
      /usr/bin
      /usr/local/bin
    NO_DEFAULT_PATH
  )
  # Fall back to system search
  if(NOT CLANG_TIDY_EXECUTABLE)
    find_program(CLANG_TIDY_EXECUTABLE NAMES clang-tidy)
  endif()
  
  if(CLANG_TIDY_EXECUTABLE)
    message(STATUS "clang-tidy found: ${CLANG_TIDY_EXECUTABLE}")
    set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXECUTABLE};--config-file=${CMAKE_SOURCE_DIR}/.clang-tidy")
  else()
    message(WARNING "clang-tidy not found - static analysis disabled")
  endif()
endif()

# Create library for core SPH functionality (used by both executable and tests)
add_library(sph_lib STATIC)
target_compile_features(sph_lib PUBLIC cxx_std_17)
target_compile_options(sph_lib
  PUBLIC
    # Strictest warning level
    -Wall
    -Wextra
    -Wpedantic
    -Wconversion
    -Wsign-conversion
    -Wshadow
    -Wnon-virtual-dtor
    -Wold-style-cast
    -Wcast-align
    -Wunused
    -Woverloaded-virtual
    -Wnull-dereference
    -Wdouble-promotion
    -Wformat=2
    # Disable specific warnings for scientific computing
    -Wno-sign-compare
    $<$<CXX_COMPILER_ID:GNU>:-Wno-maybe-uninitialized>
    -Wno-float-conversion
    -Wno-double-promotion
    # Optimizations
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
target_compile_definitions(sph PRIVATE DIM=1)
target_link_libraries(sph PRIVATE sph_lib)
target_sources(sph PRIVATE src/main.cpp)

# Create 2D executable
add_executable(sph2d)
target_compile_definitions(sph2d PRIVATE DIM=2)
target_link_libraries(sph2d PRIVATE sph_lib)
target_sources(sph2d PRIVATE src/main.cpp)

# Create 3D executable
add_executable(sph3d)
target_compile_definitions(sph3d PRIVATE DIM=3)
target_link_libraries(sph3d PRIVATE sph_lib)
target_sources(sph3d PRIVATE src/main.cpp)

add_subdirectory(include)
add_subdirectory(src)

# Testing
option(BUILD_TESTING "Build tests" ON)
if(BUILD_TESTING)
  add_subdirectory(tests)
endif()

# Installation targets
install(TARGETS sph sph2d sph3d
  RUNTIME DESTINATION bin
)

install(DIRECTORY include/
  DESTINATION include
  FILES_MATCHING PATTERN "*.hpp" PATTERN "*.tpp"
)
