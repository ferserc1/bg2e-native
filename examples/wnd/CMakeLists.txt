set(SOURCE_FILES main.cpp)
include_directories(${Vulkan_INCLUDE_DIRS})

add_executable(example-wnd ${SOURCE_FILES})
target_link_libraries(example-wnd bg2-file)
target_link_libraries(example-wnd bg2-base)
target_link_libraries(example-wnd bg2-math)
target_link_libraries(example-wnd bg2-db)
target_link_libraries(example-wnd bg2-wnd)
target_link_libraries(example-wnd bg2-render)
target_link_libraries(example-wnd ${Vulkan_LIBRARIES})

# System macOS frameworks
if(APPLE)
    target_link_libraries(example-wnd
        "-framework Cocoa"
        "-framework OpenGL"
        "-framework IOKit"
        "-framework CoreVideo")
endif()

set_target_properties(
    example-wnd PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>/")

# Data directory
add_custom_command(TARGET example-wnd POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_directory "${PROJECT_SOURCE_DIR}/data" "${CMAKE_BINARY_DIR}/$<CONFIG>/data"
)