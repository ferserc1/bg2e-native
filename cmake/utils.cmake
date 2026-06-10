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

        string(FIND "${NAME}" ".rgen." IS_RGEN)
        string(FIND "${NAME}" ".rmiss." IS_RMISS)
        string(FIND "${NAME}" ".rchit." IS_RCHIT)
        string(FIND "${NAME}" ".rahit." IS_RAHIT)
        string(FIND "${NAME}" ".rint." IS_RINT)

        if(IS_RGEN GREATER -1 OR IS_RMISS GREATER -1 OR IS_RCHIT GREATER -1 OR IS_RAHIT GREATER -1 OR IS_RINT GREATER -1)
            add_custom_command(
                OUTPUT ${SPV}
                COMMAND "${GLSLANG_PATH}" -V --target-env vulkan1.2 "${SH}" -o "${SPV}"
                DEPENDS ${SH}
                COMMENT "Building RT shader ${SH}"
                VERBATIM
            )
        else()
            add_custom_command(
                OUTPUT ${SPV}
                COMMAND "${GLSLANG_PATH}" -V "${SH}" -o "${SPV}"
                DEPENDS ${SH}
                COMMENT "Building shader ${SH}"
                VERBATIM
            )
        endif()

        list(APPEND COMPILED_SHADERS ${SPV})
    endforeach()

    add_custom_target(${TARGET_NAME}_shaders ALL DEPENDS ${COMPILED_SHADERS})

    add_dependencies(${TARGET_NAME} ${TARGET_NAME}_shaders)
endfunction()

# Compile Metal shaders from SRC_PATH (*.metal) to DST_PATH (*.metallib)
# MACOSX Build tools must be installed (Command Line Tools or Xcode)
# Uses POST_BUILD commands so that the compiled shaders are available before
# any subsequent POST_BUILD steps (e.g. bundle_resources).
# No-op on non-Apple platforms.
function(compile_metal_shaders
    TARGET_NAME
    SRC_PATH
    DST_PATH
)
    if(NOT APPLE)
        return()
    endif()

    file(GLOB MSL_SRC "${SRC_PATH}/*.metal")

    if(NOT MSL_SRC)
        return()
    endif()

    add_custom_command(
        TARGET ${TARGET_NAME}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${DST_PATH}"
    )

    foreach(MSH ${MSL_SRC})
        get_filename_component(NAME "${MSH}" NAME)
        string(REPLACE ".metal" "" BASE "${NAME}")
        set(AIR "${DST_PATH}/${BASE}.air")
        set(MTLIB "${DST_PATH}/${BASE}.metallib")

        add_custom_command(
            TARGET ${TARGET_NAME}
            POST_BUILD
            COMMAND xcrun -sdk macosx metal -c -o "${AIR}" "${MSH}"
            COMMENT "Compiling metal shader ${MSH}"
            VERBATIM
        )

        add_custom_command(
            TARGET ${TARGET_NAME}
            POST_BUILD
            COMMAND xcrun -sdk macosx metallib -o "${MTLIB}" "${AIR}"
            COMMENT "Building metal library ${MTLIB}"
            VERBATIM
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
            SUBPATH "lib"
        )

        # MoltanVk and layers
        bundle_libs(${TARGET_NAME} "lib"
            "${VULKAN_SDK_PATH}/lib/libMoltenVK.dylib"
            "${VULKAN_SDK_PATH}/lib/libVkLayer_khronos_validation.dylib"
            "${VULKAN_SDK_PATH}/lib/libVkLayer_khronos_shader_object.dylib"
            "${VULKAN_SDK_PATH}/lib/libVkLayer_khronos_profiles.dylib"
            "${VULKAN_SDK_PATH}/lib/libVkLayer_gfxreconstruct.dylib"
            "${VULKAN_SDK_PATH}/lib/libVkLayer_api_dump.dylib"
        )

        # symbolic link to LIBVULKAN_NAME
        add_custom_command(
            TARGET ${TARGET_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E create_symlink
            "$<TARGET_FILE_DIR:${TARGET_NAME}>/../lib/${LIBVULKAN_NAME}"
            "$<TARGET_FILE_DIR:${TARGET_NAME}>/../lib/libvulkan.1.dylib"
        )
    endif(APPLE)
endfunction()

# Bundle the application with its resources and libraries
#  usage: bundle_app(
#              TARGET_NAME my_app
#              SHADERS_SRC ${BG2E_SHADERS}  # optional
#       )
function(bundle_app)
    set(options)
    set(oneValueArgs TARGET_NAME SHADERS_SRC)
    cmake_parse_arguments(BL "${options}" "${oneValueArgs}" "" ${ARGN})

    file(GLOB APP_SOURCE_FILES
        "${CMAKE_CURRENT_LIST_DIR}/src/*.cpp"
    )
    file(GLOB APP_HEADER_FILES
        "${CMAKE_CURRENT_LIST_DIR}/src/*.hpp"
    )

    set(APP_HEADER_PATH "${CMAKE_CURRENT_LIST_DIR}/src")

    if(APPLE)
        add_executable(${APP_TARGET_NAME} MACOSX_BUNDLE ${APP_SOURCE_FILES})
    else()
        add_executable(${APP_TARGET_NAME} ${APP_SOURCE_FILES})
    endif()

    target_include_directories(${APP_TARGET_NAME} PUBLIC
        ${APP_HEADER_PATH}
        ${BG2E_INCLUDE_PATH}
        ${VULKAN_INCLUDE_PATH}
    )
    target_link_libraries(${APP_TARGET_NAME} PRIVATE
        ${BG2E_LIB_TARGET}
        ${VULKAN_LIB}  # Some apps need to access Vulkan directly
    )

    if(WIN32)
        target_link_libraries(${APP_TARGET_NAME} PRIVATE ${SDL2_LIBRARIES})
    endif()

    set_target_properties(${APP_TARGET_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${PRODUCT_DIR}"
        LIBRARY_OUTPUT_DIRECTORY "${PRODUCT_DIR}"
        ARCHIVE_OUTPUT_DIRECTORY "${PRODUCT_DIR}"
    )

    # Compile the shaders if SHADERS_SRC is set
    if(BL_SHADERS_SRC)
        set(APP_SHADERS_DST_PATH "${PRODUCT_DIR}/${APP_TARGET_NAME}_resources/app_shaders")
        message(STATUS "Building app shaders: ${APP_SHADERS_DST_PATH}")
        add_custom_command(
            TARGET ${APP_TARGET_NAME}
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${APP_SHADERS_DST_PATH}"
        )
        compile_shaders(${APP_TARGET_NAME} ${VULKAN_SDK} ${BL_SHADERS_SRC} ${APP_SHADERS_DST_PATH})
        bundle_resources(TARGET_NAME ${APP_TARGET_NAME} SRC_PATH ${APP_SHADERS_DST_PATH} SUBPATH "shaders/${APP_TARGET_NAME}")
    endif()


    bundle_lib(TARGET_NAME ${APP_TARGET_NAME} LIB_PATH ${SDL2_LIBRARY})
    bundle_lib(TARGET_NAME ${APP_TARGET_NAME} LIB_PATH "$<TARGET_FILE:${BG2E_LIB_TARGET}>")

    # Engine shaders
    bundle_resources(TARGET_NAME ${BL_TARGET_NAME} SRC_PATH ${BG2E_SHADER_DIR} SUBPATH "shaders")

    # Assets
    bundle_resources(TARGET_NAME ${BL_TARGET_NAME} SRC_PATH ${ASSETS_PATH} SUBPATH "assets")

    # Vulkan resources
    copy_vulkan_resources(${BL_TARGET_NAME} ${VULKAN_SDK})

    # Setup rpath in macOS app bundle
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
    endif()
endfunction()

# Same as bundle_app, but links SDL2 directly (for apps that use SDL2 API)
function(bundle_app_sdl)
    set(options)
    set(oneValueArgs TARGET_NAME SHADERS_SRC)
    cmake_parse_arguments(BL "${options}" "${oneValueArgs}" "" ${ARGN})

    file(GLOB APP_SOURCE_FILES
        "${CMAKE_CURRENT_LIST_DIR}/src/*.cpp"
    )
    file(GLOB APP_HEADER_FILES
        "${CMAKE_CURRENT_LIST_DIR}/src/*.hpp"
    )

    set(APP_HEADER_PATH "${CMAKE_CURRENT_LIST_DIR}/src")

    if(APPLE)
        add_executable(${APP_TARGET_NAME} MACOSX_BUNDLE ${APP_SOURCE_FILES})
    else()
        add_executable(${APP_TARGET_NAME} ${APP_SOURCE_FILES})
    endif()

    target_include_directories(${APP_TARGET_NAME} PUBLIC
        ${APP_HEADER_PATH}
        ${BG2E_INCLUDE_PATH}
        ${VULKAN_INCLUDE_PATH}
    )
    target_link_libraries(${APP_TARGET_NAME} PRIVATE
        ${BG2E_LIB_TARGET}
        ${VULKAN_LIB}
        ${SDL2_LIBRARIES}
    )

    set_target_properties(${APP_TARGET_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${PRODUCT_DIR}"
        LIBRARY_OUTPUT_DIRECTORY "${PRODUCT_DIR}"
        ARCHIVE_OUTPUT_DIRECTORY "${PRODUCT_DIR}"
    )

    # Compile the shaders if SHADERS_SRC is set
    if(BL_SHADERS_SRC)
        set(APP_SHADERS_DST_PATH "${PRODUCT_DIR}/${APP_TARGET_NAME}_resources/app_shaders")
        message(STATUS "Building app shaders: ${APP_SHADERS_DST_PATH}")
        add_custom_command(
            TARGET ${APP_TARGET_NAME}
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${APP_SHADERS_DST_PATH}"
        )
        compile_shaders(${APP_TARGET_NAME} ${VULKAN_SDK} ${BL_SHADERS_SRC} ${APP_SHADERS_DST_PATH})
        bundle_resources(TARGET_NAME ${APP_TARGET_NAME} SRC_PATH ${APP_SHADERS_DST_PATH} SUBPATH "shaders/${APP_TARGET_NAME}")
    endif()


    bundle_lib(TARGET_NAME ${APP_TARGET_NAME} LIB_PATH ${SDL2_LIBRARY})
    bundle_lib(TARGET_NAME ${APP_TARGET_NAME} LIB_PATH "$<TARGET_FILE:${BG2E_LIB_TARGET}>")

    # Engine shaders
    bundle_resources(TARGET_NAME ${BL_TARGET_NAME} SRC_PATH ${BG2E_SHADER_DIR} SUBPATH "shaders")

    # Assets
    bundle_resources(TARGET_NAME ${BL_TARGET_NAME} SRC_PATH ${ASSETS_PATH} SUBPATH "assets")

    # Vulkan resources
    copy_vulkan_resources(${BL_TARGET_NAME} ${VULKAN_SDK})

    # Setup rpath in macOS app bundle
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
    endif()
endfunction()
