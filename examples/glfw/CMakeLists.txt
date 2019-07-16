set(SOURCE_FILES main.cpp)
include_directories(${GLFW_INCLUDE_DIRS})
include_directories(${Vulkan_INCLUDE_DIRS})

add_executable(example-glfw ${SOURCE_FILES})
target_link_libraries(example-glfw bg2-file)
target_link_libraries(example-glfw bg2-base)
target_link_libraries(example-glfw bg2-math)
target_link_libraries(example-glfw bg2-db)
target_link_libraries(example-glfw bg2-wnd)
target_link_libraries(example-glfw bg2-render)
target_link_libraries(example-glfw ${GLFW_LIBRARIES})
target_link_libraries(example-glfw ${Vulkan_LIBRARIES})

# System macOS frameworks
if(APPLE)
    target_link_libraries(example-glfw
        "-framework Cocoa"
        "-framework OpenGL"
        "-framework IOKit"
        "-framework CoreVideo")
endif()

set_target_properties(
    example-glfw PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/${CMAKE_BUILD_TYPE}/")

# Data directory
add_custom_command(TARGET example-glfw POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_directory "${PROJECT_SOURCE_DIR}/data" "${CMAKE_BINARY_DIR}/$<CONFIG>/data"
)