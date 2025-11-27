
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

# Copy a library to the macOS application bundle
#  usage: bundle_library(
#              TARGET_NAME my_app
#              LIB_PATH ${SDL2_LIBRARY}
#              SUBPATH lib  # optional, default: lib
#       )
function(bundle_lib)
    if(APPLE)
        set(options)
        set(oneValueArgs TARGET_NAME LIB_PATH SUBPATH)
        cmake_parse_arguments(BL "${options}" "${oneValueArgs}" "" ${ARGN})

        # Default value
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
#  usage:
#  bundle_libs(
#      ${TARGET_NAME}
#      "lib"                       # destination inside the bundle
#      "${VULKAN_SDK_PATH}/lib/libMoltenVK.dylib"
#      "${VULKAN_SDK_PATH}/lib/libVkLayer_khronos_validation.dylib"
#      "${VULKAN_SDK_PATH}/lib/libvulkan.1.dylib"
#  )
function(bundle_libs TARGET_NAME SUBPATH)
    # The rest of the arguments are the libs
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

# Copy a directory into the resources folder. The resources folder is the 
# "Content/Resources" folder of the macOS bundle, and the application
# binary folder in Linux and Windows.
#  usage: bundle_library(
#              TARGET_NAME my_app
#              SRC_PATH ${BG2E_SHADERS}
#              SUBPATH "shaders"  # Path inside the resources folder
#       )
function(bundle_resources)
    set(options)
    set(oneValueArgs TARGET_NAME SRC_PATH SUBPATH)
    cmake_parse_arguments(BL "${options}" "${oneValueArgs}" "" ${ARGN})

    # Default value
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

# Copy Vulkan resources to the macOS bundle.
# Does nothing on other systems, but can be called
# without using if(APPLE)
function(copy_vulkan_resources
    TARGET_NAME
    VULKAN_SDK_PATH
)
    if (APPLE)
        # Get vulkan dylib file name
        file(GLOB LIBVULKAN_FILES "${VULKAN_SDK_PATH}/lib/libvulkan.*.dylib")
        list(GET LIBVULKAN_FILES 0 LIBVULKAN_FULL)
        get_filename_component(LIBVULKAN_NAME "${LIBVULKAN_FULL}" NAME)

        # Config files
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

        # Main Vulkan libraries
        bundle_lib(
            TARGET_NAME ${TARGET_NAME}
            LIB_PATH "${VULKAN_SDK_PATH}/lib/${LIBVULKAN_NAME}"
            SUBPATH "Frameworks"
        )

        # MoltanVk and layers
        bundle_libs(${TARGET_NAME} "lib"
            "${VULKAN_SDK_PATH}/lib/libMoltenVK.dylib"
            "${VULKAN_SDK_PATH}/lib/libVkLayer_khronos_validation.dylib"
            "${VULKAN_SDK_PATH}/lib/libVkLayer_khronos_synchronization2.dylib"
            "${VULKAN_SDK_PATH}/lib/libVkLayer_khronos_shader_object.dylib"
            "${VULKAN_SDK_PATH}/lib/libVkLayer_khronos_profiles.dylib"
            "${VULKAN_SDK_PATH}/lib/libVkLayer_gfxreconstruct.dylib"
            "${VULKAN_SDK_PATH}/lib/libVkLayer_api_dump.dylib"
        )

        # symbolic link to LIBVULKAN_NAME
        add_custom_command(
            TARGET ${TARGET_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E create_symlink
            "$<TARGET_FILE_DIR:${TARGET_NAME}>/../Frameworks/${LIBVULKAN_NAME}"
            "$<TARGET_FILE_DIR:${TARGET_NAME}>/../Frameworks/libvulkan.1.dylib"
        )
    endif(APPLE)
endfunction()

# Bundle the application with its resources and libraries
#  usage: bundle_app(
#              TARGET_NAME my_app
#              ASSETS_PATH ${BG2E_ASSETS}
#              SHADERS_SRC ${BG2E_SHADERS}  # optional
#       )
function(bundle_app)
    set(options)
    set(oneValueArgs TARGET_NAME ASSETS_PATH SHADERS_SRC)
    cmake_parse_arguments(BL "${options}" "${oneValueArgs}" "" ${ARGN})

    # TODO: Compile the shaders if SHADERS_SRC is set
    if(BL_SHADERS_SRC)

    endif()


    bundle_lib(TARGET_NAME ${BL_TARGET_NAME} LIB_PATH ${SDL2_LIBRARY})
    bundle_lib(TARGET_NAME ${BL_TARGET_NAME} LIB_PATH "$<TARGET_FILE:sdl_wrapper>")
    bundle_resources(TARGET_NAME ${BL_TARGET_NAME} SRC_PATH ${BG2E_SHADERS} SUBPATH "shaders")
    bundle_resources(TARGET_NAME ${BL_TARGET_NAME} SRC_PATH ${ASSETS_PATH} SUBPATH "assets")
    copy_vulkan_resources(${BL_TARGET_NAME} ${VULKAN_SDK})
    if (APPLE)
        set_target_properties(${BL_TARGET_NAME} PROPERTIES
            INSTALL_RPATH "@executable_path/../lib"
            BUILD_WITH_INSTALL_RPATH TRUE
        )
    endif()
endfunction()
