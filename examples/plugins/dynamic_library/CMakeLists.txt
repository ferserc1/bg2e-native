cmake_minimum_required(VERSION 3.18)

add_library(dylib_example_lib SHARED plugin/getMessage.cpp)
set_target_properties(dylib_example_lib PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/plugin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/plugin"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/plugin"
)

set(APP_TARGET_NAME dylib_example_app)
bundle_app_sdl(TARGET_NAME ${APP_TARGET_NAME})
target_compile_definitions(dylib_example_app PRIVATE
    DYLIB_EXAMPLE_FILENAME="$<TARGET_FILE_NAME:dylib_example_lib>"
)

if(APPLE)
    set(PLUGIN_DESTINATION "$<TARGET_FILE_DIR:dylib_example_app>/../Resources/plugins")
else()
    set(PLUGIN_DESTINATION "$<TARGET_FILE_DIR:dylib_example_app>/plugins")
endif()

# Refresh on every app build, including when only the plugin has changed.
add_custom_target(dylib_example_app_plugin
    COMMAND ${CMAKE_COMMAND} -E make_directory "${PLUGIN_DESTINATION}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:dylib_example_lib>" "${PLUGIN_DESTINATION}/$<TARGET_FILE_NAME:dylib_example_lib>"
    DEPENDS dylib_example_lib
    VERBATIM
)
add_dependencies(dylib_example_app dylib_example_lib dylib_example_app_plugin)
