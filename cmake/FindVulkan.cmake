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
