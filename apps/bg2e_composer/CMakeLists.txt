cmake_minimum_required(VERSION 3.18)

set(APP_TARGET_NAME bg2e_composer)
bundle_app_sdl(TARGET_NAME ${APP_TARGET_NAME})

# The shared bundle helper collects only top-level sources. Extend this app's
# target with nested code, and track additions/removals for automatic reloads.
file(GLOB_RECURSE COMPOSER_CODE_FILES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
)
get_target_property(COMPOSER_EXISTING_SOURCES ${APP_TARGET_NAME} SOURCES)
list(REMOVE_ITEM COMPOSER_CODE_FILES ${COMPOSER_EXISTING_SOURCES})
target_sources(${APP_TARGET_NAME} PRIVATE ${COMPOSER_CODE_FILES})

if(APPLE)
    set_target_properties(${APP_TARGET_NAME} PROPERTIES
        MACOSX_BUNDLE_GUI_IDENTIFIER "org.bg2e.apps.draw.bg2e-composer"
        MACOSX_BUNDLE_BUNDLE_NAME "bg2e Composer (draw)"
    )
endif()
