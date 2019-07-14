
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

file(GLOB_RECURSE BG2_MATH_INCLUDES "include/bg2math/*.hpp")
file(GLOB_RECURSE BG2_MATH_SOURCES "src/*.cpp")

source_group("Header Files" FILES ${BG2_MATH_INCLUDES})
add_library(bg2-math SHARED ${BG2_MATH_SOURCES} ${BG2_MATH_INCLUDES})

target_compile_definitions(bg2-math PRIVATE BG2_LIBRARY=1)
