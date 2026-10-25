cmake_minimum_required(VERSION 3.23)

set(APP_TARGET_NAME tutorial_plugins)
if(APPLE)
    bundle_app_sdl(TARGET_NAME ${APP_TARGET_NAME})
else()
    bundle_app_sdl(TARGET_NAME ${APP_TARGET_NAME}
        OUTPUT_DIRECTORY "${PRODUCT_DIR}/examples/tutorial_plugins")
endif()

bg2e_add_plugin(
    TARGET_NAME tutorial_components_plugin
    APP_TARGET ${APP_TARGET_NAME}
    SOURCES
        plugin/TutorialPlugin.cpp
        plugin/RotationComponent.cpp
        plugin/ReflectionSamplesComponent.cpp)
