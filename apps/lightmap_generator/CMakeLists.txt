cmake_minimum_required(VERSION 3.18)

set(APP_TARGET_NAME lightmap_generator)
bundle_app(TARGET_NAME ${APP_TARGET_NAME})
if(APPLE)
    set_target_properties(${APP_TARGET_NAME} PROPERTIES
        MACOSX_BUNDLE_GUI_IDENTIFIER "org.bg2e.apps.draw.lightmap-generator"
        MACOSX_BUNDLE_BUNDLE_NAME "Lightmap Generator (draw)"
    )
endif()
