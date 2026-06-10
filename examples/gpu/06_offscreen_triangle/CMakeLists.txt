cmake_minimum_required(VERSION 3.18)

set(APP_TARGET_NAME gpu_offscreen_triangle)
set(APP_SHADERS "${CMAKE_CURRENT_SOURCE_DIR}/shaders/glsl")

# Metal shaders (macOS only, no-op elsewhere)
# Must be called before bundle_app so POST_BUILD commands are ordered correctly
set(METAL_SHADERS_SRC "${CMAKE_CURRENT_SOURCE_DIR}/shaders/metal")
set(METAL_SHADERS_DST "${PRODUCT_DIR}/${APP_TARGET_NAME}_resources/app_shaders/metal")

bundle_app(
    TARGET_NAME ${APP_TARGET_NAME}
    SHADERS_SRC ${APP_SHADERS}
)

if(APPLE)
    compile_metal_shaders(${APP_TARGET_NAME} "${METAL_SHADERS_SRC}" "${METAL_SHADERS_DST}")
    bundle_resources(TARGET_NAME ${APP_TARGET_NAME} SRC_PATH ${METAL_SHADERS_DST} SUBPATH "shaders/${APP_TARGET_NAME}/metal")
endif()
