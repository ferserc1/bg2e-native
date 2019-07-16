
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

file(GLOB_RECURSE BG2_RENDER_INCLUDES "include/bg2render/*.hpp")
file(GLOB_RECURSE BG2_RENDER_SOURCES "src/*.cpp")

include_directories(${Vulkan_INCLUDE_DIRS})

source_group("Header Files" FILES ${BG2_RENDER_INCLUDES})
add_library(bg2-render SHARED ${BG2_RENDER_SOURCES} ${BG2_RENDER_INCLUDES})
target_link_libraries(bg2-render ${Vulkan_LIBRARIES})

target_compile_definitions(bg2-render PRIVATE BG2_LIBRARY=1)

# System macOS frameworks
if(APPLE)
    target_link_libraries(bg2-render
        "-framework Metal"
        "-framework CoreFoundation"
        "-framework Foundation"
        "-framework QuartzCore"
        "-framework IOSurface"
        "-framework IOKit"
        "-framework AppKit")
endif()
