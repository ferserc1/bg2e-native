
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

file(GLOB_RECURSE BG2_DB_INCLUDES "include/bg2db/*")
file(GLOB_RECURSE BG2_DB_SOURCES "src/*.cpp")

source_group("Header Files" FILES ${BG2_DB_INCLUDES})
add_library(bg2-db SHARED ${BG2_DB_SOURCES} ${BG2_DB_INCLUDES})
target_link_libraries(bg2-db bg2-base)
target_link_libraries(bg2-db bg2-file)
target_compile_definitions(bg2-db PRIVATE BG2_LIBRARY=1)
