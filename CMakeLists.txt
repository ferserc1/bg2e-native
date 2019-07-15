cmake_minimum_required(VERSION 3.4)
project(bg2e-cpp)

set(CMAKE_CXX_STANDARD 11)

include(FindPackageHandleStandardArgs)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})


find_package(Vulkan REQUIRED)
find_package(GLFW REQUIRED)

if (Vulkan_FOUND)
    message("Vulkan found:")
    message("  Include directory:" ${Vulkan_INCLUDE_DIRS})
    message("  Library directory:" ${Vulkan_LIBRARIES})
endif()

if (GLFW_FOUND)
    message("GLFW found:")
    message("  Include directory:" ${GLFW_INCLUDE_DIRS})
    message("  Library directory:" ${GLFW_LIBRARIES})
endif()

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
include_directories("${PROJECT_SOURCE_DIR}/bg2-base/include")
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

# bg2-wnd library
include_directories("${PROJECT_SOURCE_DIR}/bg2-wnd/include")
add_subdirectory(bg2-wnd)

# Executable test
add_subdirectory(ExecutableTest)

# Examples
add_subdirectory(examples/glfw)