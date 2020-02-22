set(SOURCE_FILES main.cpp)
include_directories(${Vulkan_INCLUDE_DIRS})

add_executable(example-vk-instance ${SOURCE_FILES})
target_link_libraries(example-vk-instance bg2-file)
target_link_libraries(example-vk-instance bg2-base)
target_link_libraries(example-vk-instance bg2-math)
target_link_libraries(example-vk-instance bg2-db)
target_link_libraries(example-vk-instance bg2-wnd)
target_link_libraries(example-vk-instance bg2-render)
target_link_libraries(example-vk-instance ${Vulkan_LIBRARIES})

# System macOS frameworks
if(APPLE)
    target_link_libraries(example-vk-instance
        "-framework Cocoa"
        "-framework OpenGL"
        "-framework IOKit"
        "-framework CoreVideo")
endif()

set_target_properties(
    example-vk-instance PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>/")

# Data directory
add_custom_command(TARGET example-vk-instance POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_directory "${PROJECT_SOURCE_DIR}/data" "${CMAKE_BINARY_DIR}/$<CONFIG>/data"
)