cmake_minimum_required(VERSION 4.0)

include("${CMAKE_SOURCE_DIR}/cmake/utils.cmake")

set(CMAKE_CXX_STANDARD 20)

# Compiled dependencies
# - BG2E_THIRD_PARTY_INCLUDE_PATH
# - BG2E_THIRD_PARTY_SRC
include("${CMAKE_CURRENT_LIST_DIR}/cmake/deps.cmake")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
find_package(SDL2 REQUIRED)

# bg2e variables
set(BG2E_TARGET_NAME bg2e)

file(GLOB_RECURSE BG2E_INCLUDE_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp"
)
file(GLOB_RECURSE BG2E_SRC_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)
if(APPLE)
    file(GLOB_RECURSE BG2E_MM_SRC_FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.mm"
    )
endif()

# bg2e library
add_library(${BG2E_TARGET_NAME} SHARED
    ${BG2E_SRC_FILES}
    ${BG2E_THIRD_PARTY_SRC}
    ${BG2E_MM_SRC_FILES}
)
target_include_directories(${BG2E_TARGET_NAME} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    ${BG2E_THIRD_PARTY_INCLUDE_PATH}
    ${VULKAN_INCLUDE_PATH}
    ${VMA_INCLUDE_PATH}
    ${SDL2_INCLUDE_DIRS}
)

# GTK (Linux only, required for NFD)
if(UNIX AND NOT APPLE)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

    target_include_directories(${BG2E_TARGET_NAME} PRIVATE ${GTK3_INCLUDE_DIRS})
    target_link_libraries(${BG2E_TARGET_NAME} PRIVATE ${GTK3_LIBRARIES})
    target_compile_options(${BG2E_TARGET_NAME} PRIVATE ${GTK3_CFLAGS_OTHER})

    find_package(OpenGL REQUIRED)
    target_link_libraries(${BG2E_TARGET_NAME} PRIVATE OpenGL::GL)
elseif(APPLE)
    target_link_libraries(${BG2E_TARGET_NAME} PRIVATE
        "-framework UniformTypeIdentifiers"
        "-framework Cocoa"
        "-framework AppKit"
        "-framework Metal"
        "-framework OpenGL"
    )
else()
    target_link_libraries(${BG2E_TARGET_NAME} PRIVATE "opengl32.lib")
endif()

target_link_libraries(${BG2E_TARGET_NAME} PRIVATE ${SDL2_LIBRARIES})
target_link_libraries(${BG2E_TARGET_NAME} PRIVATE ${VULKAN_LIB})


set_target_properties(${BG2E_TARGET_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${PRODUCT_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${PRODUCT_DIR}"
    ARCHIVE_OUTPUT_DIRECTORY "${PRODUCT_DIR}"
)

if(APPLE)
    set_target_properties(${BG2E_TARGET_NAME} PROPERTIES
        INSTALL_RPATH "@executable_path/../lib"
        BUILD_WITH_INSTALL_RPATH TRUE
    )
endif()

set(SRC_SHADER_DIR "${CMAKE_SOURCE_DIR}/shaders/src")
set(DST_SHADER_DIR "${PRODUCT_DIR}/shaders")
compile_shaders(
    ${BG2E_TARGET_NAME}
    ${VULKAN_SDK}
    ${SRC_SHADER_DIR}
    ${DST_SHADER_DIR}
)

# Exported variables
set(BG2E_LIB_TARGET ${BG2E_TARGET_NAME} PARENT_SCOPE)
set(BG2E_RUNTIME_BINARY "$<TARGET_FILE:${BG2E_TARGET_NAME}>" PARENT_SCOPE)
set(BG2E_INCLUDE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/include" PARENT_SCOPE)
set(BG2E_SHADER_DIR ${DST_SHADER_DIR} PARENT_SCOPE)