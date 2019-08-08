
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

file(GLOB_RECURSE BG2_RENDER_INCLUDES "include/bg2render/*.hpp")
file(GLOB_RECURSE BG2_RENDER_SOURCES "src/*.cpp")

include_directories(${Vulkan_INCLUDE_DIRS})

source_group("Header Files" FILES ${BG2_RENDER_INCLUDES})
add_library(bg2-render SHARED ${BG2_RENDER_SOURCES} ${BG2_RENDER_INCLUDES})
target_link_libraries(bg2-render ${Vulkan_LIBRARIES})
target_link_libraries(bg2-render bg2-base)
target_link_libraries(bg2-render bg2-db)
target_link_libraries(bg2-render bg2-wnd)

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

file(GLOB_RECURSE BG2_VERT_SHADERS "shaders/*.vert")
file(GLOB_RECURSE BG2_FRAG_SHADERS "shaders/*.frag")

# Build Vulkan shaders
#add_custom_command(OUTPUT )
if(WIN32)
    set(SHADER_BUILD "${Vulkan_BIN_DIR}/glslangValidator.exe")
else()
    set(SHADER_BUILD "${Vulkan_BIN_DIR}/glslangValidator")
endif()

function(build_shaders SHADER_FILES)
    foreach(SHADER_FILE ${SHADER_FILES})
        message(${SHADER_FILE})
        get_filename_component(SHADER_FILE_NAME ${SHADER_FILE} NAME)
        file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/Debug/shaders/")
        file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/Release/shaders/")
        add_custom_command(
            TARGET bg2-render POST_BUILD
            COMMAND ${SHADER_BUILD} -V ${SHADER_FILE} -o "${CMAKE_BINARY_DIR}/$<CONFIG>/shaders/${SHADER_FILE_NAME}.spv"
            DEPENDS ${SHADER_FILE}
            COMMENT "Building shader ${SHADER_FILE}"
            VERBATIM)
        message(STATUS "Generating build shader command for ${SHADER_FILE}")
    endforeach()
endfunction()

build_shaders(${BG2_VERT_SHADERS})
build_shaders(${BG2_FRAG_SHADERS})
#message(${SHADER_BUILD})