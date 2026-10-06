cmake_minimum_required(VERSION 3.18)

set(APP_TARGET_NAME test_02_uv_surface)
bundle_app(TARGET_NAME ${APP_TARGET_NAME})
target_include_directories(${APP_TARGET_NAME} PRIVATE
    "${CMAKE_SOURCE_DIR}/lib/src/bg2e/render"
)
