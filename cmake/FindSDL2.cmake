
# FindSDL2
# ---------
#
# Find SDL2 from the following sources:
# macOS/Windows: the VulkanSDK directory. The VULKAN_SDK variable is required
# Linux: default installation paths
#
# The following variables are defined:
#   SDL2_INCLUDE_DIRS   - The directory containing SDL.h
#   SDL2_LIBRARY        - The main SDL dynamic library
#   SDL2_LIBRARIES      - The SDL library, and also SDL2main library in Windows
#
# This module uses NO_DEFAULT_PATH on macOS to avoid Homebrew.

if(APPLE)
    set(_SDL2_ROOT "${VULKAN_SDK}")

    find_path(SDL2_INCLUDE_DIR SDL.h
        PATHS
            "${_SDL2_ROOT}/include/SDL2"
        NO_DEFAULT_PATH
    )

    set(_OLD_SUFFIXES "${CMAKE_FIND_LIBRARY_SUFFIXES}")
    # Force dynamic link
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".dylib")

    find_library(SDL2_LIBRARY
        NAMES SDL2 SDL2-2.0.0
        PATHS
            "${_SDL2_ROOT}/Lib"
            "${_SDL2_ROOT}/Lib/x64"
            "${_SDL2_ROOT}/lib"
        NO_DEFAULT_PATH
    )

    set(CMAKE_FIND_LIBRARY_SUFFIXES _OLD_SUFFIXES)
elseif(UNIX AND NOT APPLE)
    find_path(SDL2_INCLUDE_DIR SDL.h
        PATH_SUFFIXES SDL2
        PATHS
        /usr/include
        /usr/local/include
        /usr/include/SDL2
        /usr/local/include/SDL2
    )

    find_library(SDL2_LIBRARY
        NAMES SDL2
        PATHS
        /usr/lib
        /usr/lib64
        /usr/local/lib
        /usr/lib/x86_64-linux-gnu
    )
else()
    set(_SDL2_ROOT "$ENV{VULKAN_SDK}")
    find_path(SDL2_INCLUDE_DIR SDL.h
        PATHS
            "${_SDL2_ROOT}/Include/SDL2"
    )

    find_library(SDL2_LIBRARY
        NAMES SDL2 SDL2.lib
        PATHS
        "${_SDL2_ROOT}/Lib"
        "${_SDL2_ROOT}/Lib/x64"
        "${_SDL2_ROOT}/lib"
    )
endif()


include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SDL2 DEFAULT_MSG SDL2_INCLUDE_DIR SDL2_LIBRARY)

if(SDL2_FOUND)
    set(SDL2_INCLUDE_DIRS ${SDL2_INCLUDE_DIR})
    set(SDL2_LIBRARIES ${SDL2_LIBRARY})

    message(STATUS "SDL2 Found at ${SDL2_LIBRARY}")

    if(WIN32)
        find_library(SDL2MAIN_LIBRARY
                NAMES SDL2main SDL2main.lib
                PATHS
                "${_SDL2_ROOT}/Lib"
                "${_SDL2_ROOT}/Lib/x64"
        )

        list(APPEND SDL2_LIBRARIES  ${SDL2MAIN_LIBRARY})
    endif()
endif()

