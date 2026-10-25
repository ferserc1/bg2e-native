cmake_minimum_required(VERSION 3.23)
if(POLICY CMP0152)
    cmake_policy(SET CMP0152 NEW)
endif()

# Invoked by a dependency target on every application build, never by configure.
# Bundle executable directories may not exist before the app links. Collapse
# MacOS/../lib lexically before creating/resolving the destination directory.
cmake_path(ABSOLUTE_PATH DESTINATION NORMALIZE)
file(MAKE_DIRECTORY "${DESTINATION}")
file(REAL_PATH "${DESTINATION}" DESTINATION)

function(bg2e_copy_runtime_file SOURCE)
    get_filename_component(NAME "${SOURCE}" NAME)
    file(REAL_PATH "${SOURCE}" SOURCE_REAL)
    set(DEST_FILE "${DESTINATION}/${NAME}")
    # A first-build destination file does not exist yet. Resolve only existing
    # files; otherwise its already-resolved parent supplies the comparison path.
    if(EXISTS "${DEST_FILE}")
        file(REAL_PATH "${DEST_FILE}" DEST_REAL)
    else()
        set(DEST_REAL "${DEST_FILE}")
    endif()
    if(NOT SOURCE_REAL STREQUAL DEST_REAL)
        file(COPY_FILE "${SOURCE}" "${DEST_FILE}" ONLY_IF_DIFFERENT)
    endif()
endfunction()

bg2e_copy_runtime_file("${ENGINE_FILE}")
if(WIN32)
    find_program(DUMPBIN dumpbin HINTS "${TOOL_DIR}" REQUIRED)
    set(CMAKE_GET_RUNTIME_DEPENDENCIES_COMMAND "${DUMPBIN}")
    set(CMAKE_GET_RUNTIME_DEPENDENCIES_TOOL dumpbin)
    get_filename_component(ENGINE_DIR "${ENGINE_FILE}" DIRECTORY)
    get_filename_component(SDL_DIR "${SDL_LIBRARY}" DIRECTORY)
    file(GET_RUNTIME_DEPENDENCIES LIBRARIES "${ENGINE_FILE}"
        DIRECTORIES "${ENGINE_DIR}" "${SDK_ROOT}/Bin" "${SDK_ROOT}/bin" "${SDK_ROOT}/Lib"
            "${SDL_DIR}" "${SDL_DIR}/../Bin" "${SDL_DIR}/../bin"
        RESOLVED_DEPENDENCIES_VAR DEPENDENCIES UNRESOLVED_DEPENDENCIES_VAR MISSING
        PRE_EXCLUDE_REGEXES "api-ms-.*" "ext-ms-.*"
        POST_EXCLUDE_REGEXES ".*[/\\\\][Ww][Ii][Nn][Dd][Oo][Ww][Ss][/\\\\].*")
    if(MISSING)
        message(FATAL_ERROR "Unresolved engine runtime dependencies: ${MISSING}")
    endif()
    foreach(LIBRARY IN LISTS DEPENDENCIES)
        bg2e_copy_runtime_file("${LIBRARY}")
    endforeach()
elseif(APPLE AND EXISTS "${SDL_LIBRARY}")
    bg2e_copy_runtime_file("${SDL_LIBRARY}")
    get_filename_component(NAME "${SDL_LIBRARY}" NAME)
    file(REAL_PATH "${SDL_LIBRARY}" SDL_REAL)
    get_filename_component(REAL_NAME "${SDL_REAL}" NAME)
    if(NOT REAL_NAME STREQUAL NAME)
        bg2e_copy_runtime_file("${SDL_REAL}")
    endif()
endif()
