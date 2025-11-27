cmake_minimum_required(VERSION 4.0)
project(bg2e_native)

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

# Output directory
set(PRODUCT_DIR "${CMAKE_SOURCE_DIR}/bin/${PLATFORM_NAME}")
file(MAKE_DIRECTORY "${PRODUCT_DIR}")

add_subdirectory(lib)
#add_subdirectory(app)
#add_subdirectory(examples)


