cmake_minimum_required(VERSION 3.18)

set(APP_TARGET_NAME bg2e_composer)
bundle_app(TARGET_NAME ${APP_TARGET_NAME})

target_include_directories(${APP_TARGET_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/third_party/cpp-httplib
)