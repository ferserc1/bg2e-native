cmake_minimum_required(VERSION 4.0)
project(bg2e_native)

# VulkanSDK path
if(WIN32)
    set(VULKAN_SDK "$ENV{VULKAN_SDK}" CACHE PATH "Path to VulkanSDK")
    set(PLATFORM_NAME "windows")
elseif(APPLE)
    set(VULKAN_SDK "" CACHE PATH "Path to VulkanSDK")
    set(PLATFORM_NAME "macos")
else()
    set(VULKAN_SDK "$ENV{VULKAN_SDK}" CACHE PATH "Path to VulkanSDK")
    set(PLATFORM_NAME "linux")
endif()

# Validate VulkanSDK
if(NOT VULKAN_SDK OR NOT EXISTS "${VULKAN_SDK}")
    message(FATAL_ERROR
        "\n\nVulkanSDK no está configurado.\n"
        "Por favor establece la ruta manualmente en CMake:\n"
        "  -DVULKAN_SDK=/ruta/al/VulkanSDK\n\n"
        "Plataforma detectada: ${PLATFORM_NAME}\n"
    )
endif()

message(STATUS "Vulkan SDK found at: ${VULKAN_SDK}")
