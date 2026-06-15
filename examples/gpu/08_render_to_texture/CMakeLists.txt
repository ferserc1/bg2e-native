cmake_minimum_required(VERSION 3.18)

set(APP_TARGET_NAME gpu_render_to_texture)
set(APP_SHADERS_SRC "${CMAKE_CURRENT_SOURCE_DIR}/shaders")
set(APP_SHADERS_DST "${PRODUCT_DIR}/${APP_TARGET_NAME}_resources/app_shaders")

set(METAL_SHADERS_DST "${PRODUCT_DIR}/${APP_TARGET_NAME}_resources/app_shaders/metal")

bundle_app_sdl(TARGET_NAME ${APP_TARGET_NAME})

compile_shaders_shaderlib(${APP_TARGET_NAME} ${VULKAN_SDK} "${APP_SHADERS_SRC}" "${APP_SHADERS_DST}")
bundle_resources(TARGET_NAME ${APP_TARGET_NAME} SRC_PATH ${APP_SHADERS_DST} SUBPATH "shaders/${APP_TARGET_NAME}")

if(APPLE)
    compile_metal_shaders(${APP_TARGET_NAME} "${APP_SHADERS_SRC}" "${METAL_SHADERS_DST}")
    bundle_resources(TARGET_NAME ${APP_TARGET_NAME} SRC_PATH ${METAL_SHADERS_DST} SUBPATH "shaders/${APP_TARGET_NAME}")
endif()
