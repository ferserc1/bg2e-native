# The VULKAN_SDK variable must be defined
# Exports:
# - VULKAN_INCLUDE_PATH Include directory
# - VULKAN_LIB          Link library (.lib/.so/.dylib)
# - VULKAN_LIB_PATH     libraries path
# - VULKAN_LIB_NAME     Binary library name (example: libvulkan.1.4.313.dylib)
# - VULKAN_LIB_VERSION  Binary library version (example: 1.4.313)

if(APPLE)
    set(VULKAN_INCLUDE_PATH "${VULKAN_SDK}/include")

    set(VULKAN_LIB_PATH "${VULKAN_SDK}/lib")
    file(GLOB VULKAN_LIB_FILES
        "${VULKAN_LIB_PATH}/libvulkan.*.dylib"
    )
    list(GET VULKAN_LIB_FILES 0 VULKAN_LIB)

    get_filename_component(VULKAN_LIB_NAME "${VULKAN_LIB}" NAME)

    string(REGEX REPLACE "^libvulkan\\.([0-9\\.]+)\\.dylib$" "\\1"
        VULKAN_LIB_VERSION "${VULKAN_LIB_NAME}"
    )
elseif(UNIX AND NOT APPLE)
    set(VULKAN_INCLUDE_PATH "${VULKAN_SDK}/include")

    set(VULKAN_LIB_PATH "${VULKAN_SDK}/lib")
    file(GLOB VULKAN_LIB_FILES
        "${VULKAN_LIB_PATH}/libvulkan.so.1.*"
    )
    list(GET VULKAN_LIB_FILES 0 VULKAN_LIB)

    get_filename_component(VULKAN_LIB_NAME "${VULKAN_LIB}" NAME)

    string(REGEX REPLACE "^libvulkan\\.so\\.([0-9\\.]+)$" "\\1"
        VULKAN_LIB_VERSION "${VULKAN_LIB_NAME}"
    )
else()
    set(VULKAN_INCLUDE_PATH "${VULKAN_SDK}/Include")

    set(VULKAN_LIB_PATH "${VULKAN_SDK}/Lib")
    set(VULKAN_LIB_NAME "vulkan-1.lib")
    set(VULKAN_LIB "${VULKAN_LIB_PATH}/${VULKAN_LIB_NAME}")
    set(VULKAN_LIB_VERSION "1")
endif()

message(STATUS "VULKAN_INCLUDE_PATH: ${VULKAN_INCLUDE_PATH}")
message(STATUS "VULKAN_LIB: ${VULKAN_LIB}")
message(STATUS "VULKAN_LIB_PATH: ${VULKAN_LIB_PATH}")
message(STATUS "VULKAN_LIB_NAME: ${VULKAN_LIB_NAME}")
message(STATUS "VULKAN_LIB_VERSION: ${VULKAN_LIB_VERSION}")

# Set Vulkan_VERSION from vulkan_core.h so third-party packages (e.g. FidelityFX SDK) can check it.
# VK_HEADER_VERSION_COMPLETE looks like: VK_MAKE_API_VERSION(0, 1, 3, VK_HEADER_VERSION)
# VK_HEADER_VERSION is the patch number defined on its own line.
set(_vk_core_h "${VULKAN_INCLUDE_PATH}/vulkan/vulkan_core.h")
if(NOT Vulkan_VERSION AND EXISTS "${_vk_core_h}")
    file(STRINGS "${_vk_core_h}" _vk_complete REGEX "^#define VK_HEADER_VERSION_COMPLETE")
    file(STRINGS "${_vk_core_h}" _vk_patch_line REGEX "^#define VK_HEADER_VERSION [0-9]")
    string(REGEX MATCH "VK_MAKE_API_VERSION\\(0, ([0-9]+), ([0-9]+)," _ "${_vk_complete}")
    set(_vk_maj "${CMAKE_MATCH_1}")
    set(_vk_min "${CMAKE_MATCH_2}")
    string(REGEX MATCH "([0-9]+)$" _ "${_vk_patch_line}")
    if(_vk_maj AND _vk_min AND CMAKE_MATCH_1)
        set(Vulkan_VERSION "${_vk_maj}.${_vk_min}.${CMAKE_MATCH_1}")
        message(STATUS "Vulkan_VERSION: ${Vulkan_VERSION}")
    endif()
    unset(_vk_complete)
    unset(_vk_patch_line)
    unset(_vk_maj)
    unset(_vk_min)
endif()
unset(_vk_core_h)

# Create Vulkan::Vulkan imported target expected by third-party packages (e.g. FidelityFX SDK).
if(NOT TARGET Vulkan::Vulkan)
    add_library(Vulkan::Vulkan UNKNOWN IMPORTED)
    set_target_properties(Vulkan::Vulkan PROPERTIES
        IMPORTED_LOCATION             "${VULKAN_LIB}"
        INTERFACE_INCLUDE_DIRECTORIES "${VULKAN_INCLUDE_PATH}"
    )
endif()
