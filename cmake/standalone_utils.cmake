
# Standalone CMake utilities for bg2e projects
# This file is a standalone version of cmake/utils.cmake, designed to work
# with pre-compiled bg2e libraries outside the bg2e-native repository.
#
# DO NOT MODIFY the original cmake/utils.cmake - this file exists to provide
# standalone-specific functions without affecting the main build system.

# ============================================================================
# Utility functions (copied from cmake/utils.cmake, unchanged)
# ============================================================================

# Build all shaders in the folder SRC_PATH to the folder DST_PATH
function(build_shaders
    TARGET_NAME
    VULKAN_SDK_PATH
    SRC_PATH
    DST_PATH
)
    set(GLSLANG_PATH "${VULKAN_SDK_PATH}/bin/glslang")
    message(STATUS "glslang command: ${GLSLANG_PATH}")
    file(GLOB SHADERS_SRC "${SRC_PATH}/*.glsl")

    message(STATUS "build shaders from ${SRC_PATH} into ${DST_PATH}")

    add_custom_command(
        TARGET ${TARGET_NAME}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${DST_PATH}"
    )
    foreach(SH ${SHADERS_SRC})
        get_filename_component(NAME "${SH}" NAME)
        string(REPLACE ".glsl" "" BASE "${NAME}")
        add_custom_command(
            TARGET ${TARGET_NAME}
            POST_BUILD
            COMMAND "${GLSLANG_PATH}" -V "${SH}" -o "${DST_PATH}/${BASE}.spv"
        )
    endforeach()
endfunction()

function(compile_shaders
    TARGET_NAME
    VULKAN_SDK_PATH
    SRC_PATH
    DST_PATH
)
    set(COMPILED_SHADERS "")

    set(GLSLANG_PATH "${VULKAN_SDK_PATH}/bin/glslang")
    file(GLOB SHADERS_SRC "${SRC_PATH}/*.glsl")

    file(MAKE_DIRECTORY "${DST_PATH}")

    foreach(SH ${SHADERS_SRC})
        get_filename_component(NAME "${SH}" NAME)
        string(REPLACE ".glsl" "" BASE "${NAME}")
        set(SPV "${DST_PATH}/${BASE}.spv")
        add_custom_command(
            OUTPUT ${SPV}
            COMMAND "${GLSLANG_PATH}" -V "${SH}" -o "${SPV}"
            DEPENDS ${SH}
            COMMENT "Building shader ${SH}"
            VERBATIM
        )

        list(APPEND COMPILED_SHADERS ${SPV})
    endforeach()

    add_custom_target(${TARGET_NAME}_shaders ALL DEPENDS ${COMPILED_SHADERS})

    add_dependencies(${TARGET_NAME} ${TARGET_NAME}_shaders)
endfunction()

# Copy a library to the macOS application bundle
function(bundle_lib)
    if(APPLE)
        set(options)
        set(oneValueArgs TARGET_NAME LIB_PATH SUBPATH)
        cmake_parse_arguments(BL "${options}" "${oneValueArgs}" "" ${ARGN})

        if(NOT BL_SUBPATH)
            set(BL_SUBPATH "lib")
        endif()

        add_custom_command(
            TARGET ${BL_TARGET_NAME}
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_FILE_DIR:${BL_TARGET_NAME}>/../${BL_SUBPATH}"
            COMMAND ${CMAKE_COMMAND} -E copy
                "${BL_LIB_PATH}"
                "$<TARGET_FILE_DIR:${BL_TARGET_NAME}>/../${BL_SUBPATH}"
        )
    endif()
endfunction()

# Copy a list of libraries to the same destination directory inside the macOS application bundle
function(bundle_libs TARGET_NAME SUBPATH)
    set(LIB_LIST ${ARGN})

    if(APPLE)
        add_custom_command(
            TARGET ${TARGET_NAME}
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_FILE_DIR:${TARGET_NAME}>/../${SUBPATH}"
        )

        foreach(LIB_PATH IN LISTS LIB_LIST)
            add_custom_command(
                TARGET ${TARGET_NAME}
                POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                "${LIB_PATH}"
                "$<TARGET_FILE_DIR:${TARGET_NAME}>/../${SUBPATH}"
            )
        endforeach()
    endif()
endfunction()

# Copy a directory into the resources folder
function(bundle_resources)
    set(options)
    set(oneValueArgs TARGET_NAME SRC_PATH SUBPATH)
    cmake_parse_arguments(BL "${options}" "${oneValueArgs}" "" ${ARGN})

    if(NOT BL_SUBPATH)
        set(BL_SUBPATH "Resources")
    endif()

    if (APPLE)
        message(STATUS "Copy resources from ${BL_SRC_PATH} to ${BL_SUBPATH} inside bundle")

        add_custom_command(
            TARGET ${BL_TARGET_NAME}
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_FILE_DIR:${BL_TARGET_NAME}>/../Resources/${BL_SUBPATH}"
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${BL_SRC_PATH}"
                "$<TARGET_FILE_DIR:${BL_TARGET_NAME}>/../Resources/${BL_SUBPATH}"
        )
    else()
        message(STATUS "Copy resources from ${BL_SRC_PATH}")

        add_custom_command(
            TARGET ${BL_TARGET_NAME}
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_FILE_DIR:${BL_TARGET_NAME}>/${BL_SUBPATH}"
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${BL_SRC_PATH}"
                "$<TARGET_FILE_DIR:${BL_TARGET_NAME}>/${BL_SUBPATH}"
        )
    endif()
endfunction()

# Copy Vulkan resources to the macOS bundle
function(copy_vulkan_resources
    TARGET_NAME
    VULKAN_SDK_PATH
)
    if (APPLE)
        file(GLOB LIBVULKAN_FILES "${VULKAN_SDK_PATH}/lib/libvulkan.*.dylib")
        list(GET LIBVULKAN_FILES 0 LIBVULKAN_FULL)
        get_filename_component(LIBVULKAN_NAME "${LIBVULKAN_FULL}" NAME)

        bundle_resources(
            TARGET_NAME ${TARGET_NAME}
            SRC_PATH "${VULKAN_SDK_PATH}/share/vulkan/icd.d"
            SUBPATH "vulkan/icd.d"
        )
        bundle_resources(
            TARGET_NAME ${TARGET_NAME}
            SRC_PATH "${VULKAN_SDK_PATH}/share/vulkan/explicit_layer.d"
            SUBPATH "vulkan/explicit_layer.d"
        )

        bundle_lib(
            TARGET_NAME ${TARGET_NAME}
            LIB_PATH "${VULKAN_SDK_PATH}/lib/${LIBVULKAN_NAME}"
            SUBPATH "lib"
        )

        bundle_libs(${TARGET_NAME} "lib"
            "${VULKAN_SDK_PATH}/lib/libMoltenVK.dylib"
            "${VULKAN_SDK_PATH}/lib/libVkLayer_khronos_validation.dylib"
            "${VULKAN_SDK_PATH}/lib/libVkLayer_khronos_synchronization2.dylib"
            "${VULKAN_SDK_PATH}/lib/libVkLayer_khronos_shader_object.dylib"
            "${VULKAN_SDK_PATH}/lib/libVkLayer_khronos_profiles.dylib"
            "${VULKAN_SDK_PATH}/lib/libVkLayer_gfxreconstruct.dylib"
            "${VULKAN_SDK_PATH}/lib/libVkLayer_api_dump.dylib"
        )

        add_custom_command(
            TARGET ${TARGET_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E create_symlink
            "$<TARGET_FILE_DIR:${TARGET_NAME}>/../lib/${LIBVULKAN_NAME}"
            "$<TARGET_FILE_DIR:${TARGET_NAME}>/../lib/libvulkan.1.dylib"
        )
    endif(APPLE)
endfunction()

# ============================================================================
# Standalone bundle function
# ============================================================================

# Bundle a standalone application with its resources and libraries.
# This function replaces bundle_app() for standalone projects that use
# pre-compiled bg2e libraries.
#
# Required variables (set by root CMakeLists.txt):
#   - bg2e: imported target for the pre-compiled bg2e library
#   - VULKAN_SDK: path to VulkanSDK
#   - VULKAN_INCLUDE_PATH: path to Vulkan headers
#   - VULKAN_LIB: Vulkan link library
#   - SDL2_LIBRARY: SDL2 library path
#
#  usage: standalone_bundle_app(
#              TARGET_NAME my_app
#              SHADERS_SRC ${APP_SHADERS}  # optional
#       )
function(standalone_bundle_app)
    set(options)
    set(oneValueArgs TARGET_NAME SHADERS_SRC)
    cmake_parse_arguments(BL "${options}" "${oneValueArgs}" "" ${ARGN})

    set(BG2E_ROOT "${CMAKE_SOURCE_DIR}")

    file(GLOB APP_SOURCE_FILES
        "${CMAKE_CURRENT_LIST_DIR}/src/*.cpp"
    )
    file(GLOB APP_HEADER_FILES
        "${CMAKE_CURRENT_LIST_DIR}/src/*.hpp"
    )

    set(APP_HEADER_PATH "${CMAKE_CURRENT_LIST_DIR}/src")

    if(APPLE)
        add_executable(${BL_TARGET_NAME} MACOSX_BUNDLE ${APP_SOURCE_FILES})
    else()
        add_executable(${BL_TARGET_NAME} ${APP_SOURCE_FILES})
    endif()

    target_include_directories(${BL_TARGET_NAME} PUBLIC
        ${APP_HEADER_PATH}
        "${BG2E_ROOT}/include"
        ${VULKAN_INCLUDE_PATH}
    )

    target_link_libraries(${BL_TARGET_NAME} PRIVATE
        bg2e
        ${VULKAN_LIB}
    )

    if(WIN32)
        target_link_libraries(${BL_TARGET_NAME} PRIVATE ${SDL2_LIBRARIES})
    endif()

    set_target_properties(${BL_TARGET_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )

    # Compile app-specific shaders if SHADERS_SRC is set
    if(BL_SHADERS_SRC)
        set(APP_SHADERS_DST_PATH "${CMAKE_BINARY_DIR}/bin/${BL_TARGET_NAME}_resources/app_shaders")
        message(STATUS "Building app shaders: ${APP_SHADERS_DST_PATH}")
        add_custom_command(
            TARGET ${BL_TARGET_NAME}
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${APP_SHADERS_DST_PATH}"
        )
        compile_shaders(${BL_TARGET_NAME} ${VULKAN_SDK} ${BL_SHADERS_SRC} ${APP_SHADERS_DST_PATH})
        bundle_resources(TARGET_NAME ${BL_TARGET_NAME} SRC_PATH ${APP_SHADERS_DST_PATH} SUBPATH "shaders/${BL_TARGET_NAME}")
    endif()

    # Copy bg2e library alongside the executable
    if(WIN32)
        # On Windows, copy the DLL to the output directory
        add_custom_command(
            TARGET ${BL_TARGET_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${BG2E_ROOT}/lib/windows/bg2e.dll"
                "$<TARGET_FILE_DIR:${BL_TARGET_NAME}>"
        )
    elseif(APPLE)
        bundle_lib(TARGET_NAME ${BL_TARGET_NAME} LIB_PATH "${BG2E_ROOT}/lib/macos/libbg2e.dylib")
        bundle_lib(TARGET_NAME ${BL_TARGET_NAME} LIB_PATH ${SDL2_LIBRARY})
    else()
        # Linux: copy libbg2e.so alongside the executable
        add_custom_command(
            TARGET ${BL_TARGET_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${BG2E_ROOT}/lib/linux/libbg2e.so"
                "$<TARGET_FILE_DIR:${BL_TARGET_NAME}>"
        )
    endif()

    # Engine shaders
    bundle_resources(TARGET_NAME ${BL_TARGET_NAME} SRC_PATH "${BG2E_ROOT}/shaders" SUBPATH "shaders")

    # Assets
    bundle_resources(TARGET_NAME ${BL_TARGET_NAME} SRC_PATH "${BG2E_ROOT}/assets" SUBPATH "assets")

    # Vulkan resources (macOS only)
    copy_vulkan_resources(${BL_TARGET_NAME} ${VULKAN_SDK})

    # Platform-specific setup
    if (APPLE)
        target_link_libraries(${BL_TARGET_NAME} PRIVATE
            "-framework AppKit"
            "-framework Cocoa"
            "-framework Foundation"
            "-framework UniformTypeIdentifiers"
        )

        set_target_properties(${BL_TARGET_NAME} PROPERTIES
            INSTALL_RPATH "@executable_path/../lib"
            BUILD_WITH_INSTALL_RPATH TRUE
        )
    elseif(UNIX)
        # Linux: set RPATH to find libbg2e.so alongside the executable
        set_target_properties(${BL_TARGET_NAME} PROPERTIES
            INSTALL_RPATH "$ORIGIN"
            BUILD_WITH_INSTALL_RPATH TRUE
        )
    endif()
endfunction()
