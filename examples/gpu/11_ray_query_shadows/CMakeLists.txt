cmake_minimum_required(VERSION 3.18)

set(APP_TARGET_NAME gpu_ray_query_shadows)
set(APP_SHADERS_SRC "${CMAKE_CURRENT_SOURCE_DIR}/shaders")
set(APP_SHADERS_DST "${PRODUCT_DIR}/${APP_TARGET_NAME}_resources/app_shaders")

set(METAL_SHADERS_DST "${PRODUCT_DIR}/${APP_TARGET_NAME}_resources/app_shaders/metal")

bundle_app_sdl(TARGET_NAME ${APP_TARGET_NAME})

# This example uses ray queries (GL_EXT_ray_query) from the fragment shader,
# which requires SPIR-V 1.4 / Vulkan 1.2. The shared compile_shaders_shaderlib
# helper only targets Vulkan 1.2 for dedicated RT shader stages (.rgen, .rmiss,
# ...), so the ShaderLib SPIR-V is compiled here explicitly with
# --target-env vulkan1.2 and the ShaderLib entry-point naming convention.
set(GLSLANG_PATH "${VULKAN_SDK}/bin/glslang")
file(MAKE_DIRECTORY "${APP_SHADERS_DST}")

set(RQ_COMPILED_SHADERS "")
foreach(STAGE vert frag)
    if(STAGE STREQUAL "vert")
        set(ENTRY_POINT "vertMain")
    else()
        set(ENTRY_POINT "fragMain")
    endif()
    set(SRC "${APP_SHADERS_SRC}/shadow.${STAGE}.glsl")
    set(SPV "${APP_SHADERS_DST}/shadow.${STAGE}.spv")
    add_custom_command(
        OUTPUT ${SPV}
        COMMAND "${GLSLANG_PATH}" -V --target-env vulkan1.2 -e "${ENTRY_POINT}" "${SRC}" -o "${SPV}"
        DEPENDS ${SRC}
        COMMENT "Building ray query ShaderLib shader ${SRC} (entry: ${ENTRY_POINT})"
        VERBATIM
    )
    list(APPEND RQ_COMPILED_SHADERS ${SPV})
endforeach()

add_custom_target(${APP_TARGET_NAME}_shaders ALL DEPENDS ${RQ_COMPILED_SHADERS})
add_dependencies(${APP_TARGET_NAME} ${APP_TARGET_NAME}_shaders)

bundle_resources(TARGET_NAME ${APP_TARGET_NAME} SRC_PATH ${APP_SHADERS_DST} SUBPATH "shaders/${APP_TARGET_NAME}")

if(APPLE)
    compile_metal_shaders(${APP_TARGET_NAME} "${APP_SHADERS_SRC}" "${METAL_SHADERS_DST}")
    bundle_resources(TARGET_NAME ${APP_TARGET_NAME} SRC_PATH ${METAL_SHADERS_DST} SUBPATH "shaders/${APP_TARGET_NAME}")
endif()
