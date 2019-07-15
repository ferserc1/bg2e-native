
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

file(GLOB_RECURSE BG2_WND_INCLUDES "include/bg2wnd/*.hpp")
file(GLOB_RECURSE BG2_WND_SOURCES "src/*.cpp")

include_directories(${GLFW_INCLUDE_DIRS})
include_directories(${Vulkan_INCLUDE_DIRS})

source_group("Header Files" FILES ${BG2_WND_INCLUDES})
add_library(bg2-wnd SHARED ${BG2_WND_SOURCES} ${BG2_WND_INCLUDES})
target_link_libraries(bg2-wnd ${GLFW_LIBRARIES})
target_link_libraries(bg2-wnd ${Vulkan_LIBRARIES})

# System macOS frameworks
if(APPLE)
    target_link_libraries(bg2-wnd
        "-framework Cocoa"
        "-framework OpenGL"
        "-framework IOKit"
        "-framework CoreVideo")
endif()

target_compile_definitions(bg2-wnd PRIVATE BG2_LIBRARY=1)
