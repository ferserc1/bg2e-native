
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

file(GLOB_RECURSE BG2_FILE_INCLUDES "include/bg2-base/*.hpp")
file(GLOB_RECURSE BG2_FILE_SOURCES "src/*.cpp")

source_group("Header Files" FILES ${BG2_FILE_INCLUDES})
add_library(bg2-base SHARED ${BG2_FILE_SOURCES} ${BG2_FILE_INCLUDES})
