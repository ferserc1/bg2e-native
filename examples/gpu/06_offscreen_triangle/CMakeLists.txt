cmake_minimum_required(VERSION 3.18)

set(APP_TARGET_NAME gpu_offscreen_triangle)
set(APP_SHADERS_SRC "${CMAKE_CURRENT_SOURCE_DIR}/shaders")
set(APP_SHADERS_DST "${PRODUCT_DIR}/${APP_TARGET_NAME}_resources/app_shaders")

# Metal shaders (macOS only, no-op elsewhere)
# Must be called before bundle_app so POST_BUILD commands are ordered correctly
set(METAL_SHADERS_DST "${PRODUCT_DIR}/${APP_TARGET_NAME}_resources/app_shaders/metal")

# Shader compilation is handled explicitly via compile_shaders_shaderlib so that
# the SPIR-V entry-point names follow the ShaderLib convention (<stage>Main).
bundle_app(TARGET_NAME ${APP_TARGET_NAME})

compile_shaders_shaderlib(${APP_TARGET_NAME} ${VULKAN_SDK} "${APP_SHADERS_SRC}" "${APP_SHADERS_DST}")
bundle_resources(TARGET_NAME ${APP_TARGET_NAME} SRC_PATH ${APP_SHADERS_DST} SUBPATH "shaders/${APP_TARGET_NAME}")

if(APPLE)
    compile_metal_shaders(${APP_TARGET_NAME} "${APP_SHADERS_SRC}" "${METAL_SHADERS_DST}")
    bundle_resources(TARGET_NAME ${APP_TARGET_NAME} SRC_PATH ${METAL_SHADERS_DST} SUBPATH "shaders/${APP_TARGET_NAME}")
endif()
