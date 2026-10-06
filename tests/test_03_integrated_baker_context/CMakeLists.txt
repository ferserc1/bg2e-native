cmake_minimum_required(VERSION 3.18)

set(APP_TARGET_NAME test_03_integrated_baker_context)
bundle_app(TARGET_NAME ${APP_TARGET_NAME})
target_link_libraries(${APP_TARGET_NAME} PRIVATE ${SDL2_LIBRARIES})
