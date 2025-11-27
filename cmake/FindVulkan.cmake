# The VULKAN_SDK variable must be defined
# Exports:
# - VULKAN_INCLUDE_PATH Include directory
# - VULKAN_LIB          Link library (.lib/.so/.dylib)
# - VULKAN_BIN          Binary library (.dll/.so/.dylib)
# - VULKAN_LIB_PATH     libraries path
# - VULKAN_BIN_PATH     Binary libraries path
# - VULKAN_BIN_NAME     Binary library name (example: libvulkan.1.4.313.dylib)
# - VULKAN_BIN_VERSION  Binary library version (example: 1.4.313)

find_path(VULKAN_INCLUDE_PATH vulkan.h
    PATHS
        "${VULKAN_SDK}/include"
        "${VULKAN_SDK}/Include"
        "${VULKAN_SDK}/include/vulkan"
        "${VULKAN_SDK}/Include/vulkan"
)

if(APPLE)
    set(VULKAN_LIB_PATH "${VULKAN_SDK}/lib")
    file(GLOB VULKAN_LIB_FILES
        "${VULKAN_LIB_PATH}/libvulkan.*.dylib"
    )
    list(GET VULKAN_LIB_FILES 0 VULKAN_LIB)
    set(VULKAN_BIN ${VULKAN_LIB})
    set(VULKAN_BIN_PATH ${VULKAN_LIB_PATH})

    get_filename_component(VULKAN_BIN_NAME "${VULKAN_BIN}" NAME)

    string(REGEX REPLACE "^libvulkan\\.([0-9\\.]+)\\.dylib$" "\\1"
        VULKAN_BIN_VERSION "${VULKAN_BIN_NAME}"
    )
elseif(APPLE AND NOT UNIX)
    # TODO: implement this
elseif(WINDOWS)
    # TODO: implement this
endif()

message(STATUS "VULKAN_INCLUDE_PATH: ${VULKAN_INCLUDE_PATH}")
message(STATUS "VULKAN_LIB: ${VULKAN_LIB}")
message(STATUS "VULKAN_BIN: ${VULKAN_BIN}")
message(STATUS "VULKAN_LIB_PATH: ${VULKAN_LIB_PATH}")
message(STATUS "VULKAN_BIN_PATH ${VULKAN_BIN_PATH}")
message(STATUS "VULKAN_BIN_NAME: ${VULKAN_BIN_NAME}")
message(STATUS "VULKAN_BIN_VERSION: ${VULKAN_BIN_VERSION}")
