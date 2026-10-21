cmake_minimum_required(VERSION 3.18)

set(APP_TARGET_NAME bg2e_composer)
bundle_app_sdl(TARGET_NAME ${APP_TARGET_NAME})
if(APPLE)
    set_target_properties(${APP_TARGET_NAME} PROPERTIES
        MACOSX_BUNDLE_GUI_IDENTIFIER "org.bg2e.apps.draw.bg2e-composer"
        MACOSX_BUNDLE_BUNDLE_NAME "bg2e Composer (draw)"
    )
endif()
