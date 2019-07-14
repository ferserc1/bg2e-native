set(SOURCE_FILES tutorial.cxx)
add_executable(Tutorial ${SOURCE_FILES})
target_link_libraries(Tutorial bg2-file)
target_link_libraries(Tutorial bg2-base)
target_link_libraries(Tutorial bg2-math)
target_link_libraries(Tutorial bg2-db)

set_target_properties(
    Tutorial PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/${CMAKE_BUILD_TYPE}/")

# Data directory
add_custom_command(TARGET Tutorial POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_directory "${PROJECT_SOURCE_DIR}/data" "${CMAKE_BINARY_DIR}/$<CONFIG>/data"
)