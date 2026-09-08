cmake_minimum_required(VERSION 3.23)  # 3.23 required by FidelityFX SDK
project(bg2e_native)

set(CMAKE_CXX_STANDARD 20)

# VulkanSDK path
set(VULKAN_SDK "$ENV{VULKAN_SDK}" CACHE PATH "Path to VulkanSDK")
if(WIN32)
    set(PLATFORM_NAME "windows")
elseif(APPLE)
    set(PLATFORM_NAME "macos")
else()
    set(PLATFORM_NAME "linux")
endif()

# Validate VulkanSDK
if(NOT VULKAN_SDK OR NOT EXISTS "${VULKAN_SDK}")
    message(FATAL_ERROR
        "\nVulkanSDK not configured. VULKAN_SDK variable not set.\n"
        "Please, set the VulkanSDK path manually:\n"
        "  -DVULKAN_SDK=/path/to/VulkanSDK\n\n"
    )
endif()
message(STATUS "Vulkan SDK found at: ${VULKAN_SDK}")

# Find SDL2
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
find_package(SDL2 REQUIRED)

# Find Vulkan
find_package(Vulkan REQUIRED)

# Output directory
set(PRODUCT_DIR "${CMAKE_SOURCE_DIR}/bin/${PLATFORM_NAME}" CACHE PATH "Runtime output directory")
file(MAKE_DIRECTORY "${PRODUCT_DIR}")
set(ASSETS_PATH "${CMAKE_SOURCE_DIR}/assets")


if (MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
endif()

# Disable automatic codesign in macOS
set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "NO")
set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "NO")
set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "")
set(CMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "")
set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Manual")

# Enable multi processor build in Windows
if (MSVC)
    add_compile_options(/MP)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MP")
endif()

add_subdirectory(lib)
add_subdirectory(apps)
option(BG2E_BUILD_EXAMPLES "Build example applications" ON)
option(BG2E_BUILD_TESTS "Build test applications" ON)
option(BG2E_PACKAGE_APPS "Enable application distribution install rules" OFF)
if(BG2E_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
if(BG2E_BUILD_TESTS)
    add_subdirectory(tests)
endif()
if(BG2E_PACKAGE_APPS)
    include(cmake/package_apps.cmake)
endif()


