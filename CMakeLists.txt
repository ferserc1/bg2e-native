cmake_minimum_required(VERSION 3.4)
project(bg2e-cpp)

set(CMAKE_CXX_STANDARD 11)

# Version number
set (bg2e_MAJOR_VERSION 2)
set (bg2e_MINOR_VERSION 0)
set (bg2e_REV_VERSION 0)

# Configure header file to pass some of the CMake settings
configure_file (
    "${PROJECT_SOURCE_DIR}/bg2e-config.hpp.in"
    "${PROJECT_BINARY_DIR}/bg2e-config.hpp"
)

# Copy all generated library to binary directory
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

# Add the binary tree to the search path for include files
# so that we will find bg2e-config.hpp
include_directories("${PROJECT_BINARY_DIR}")

# bg2-math library
include_directories("${PROJECT_SOURCE_DIR}/bg2-math/include")
add_subdirectory(bg2-math)

# bg2-base library
include_directories("${PROJECT_SOURCE_DIR}/bg2-base/include")
add_subdirectory(bg2-base)

# bg2-db library
include_directories("${PROJECT_SOURCE_DIR}/bg2-db/include")
add_subdirectory(bg2-db)

# bg2-file library
include_directories("${PROJECT_SOURCE_DIR}/bg2-file/include")
add_subdirectory(bg2-file)


# Executable test
add_subdirectory(ExecutableTest)
