# Discover executable targets recursively; new apps need only add_subdirectory().
function(bg2e_collect_apps directory)
    get_property(targets DIRECTORY "${directory}" PROPERTY BUILDSYSTEM_TARGETS)
    foreach(target IN LISTS targets)
        get_target_property(type ${target} TYPE)
        if(type STREQUAL "EXECUTABLE")
            set_property(GLOBAL APPEND PROPERTY BG2E_PACKAGE_TARGETS ${target})
        endif()
    endforeach()
    get_property(children DIRECTORY "${directory}" PROPERTY SUBDIRECTORIES)
    foreach(child IN LISTS children)
        bg2e_collect_apps("${child}")
    endforeach()
endfunction()
bg2e_collect_apps("${CMAKE_SOURCE_DIR}/apps")
get_property(apps GLOBAL PROPERTY BG2E_PACKAGE_TARGETS)
if(NOT apps)
    message(FATAL_ERROR "No executable application targets found in apps/")
endif()
add_custom_target(bg2e_distribution DEPENDS bg2e ${apps})

if(APPLE)
    # Existing bundle_app() embeds engine, Vulkan, SDL and resources.
    install(TARGETS ${apps} BUNDLE DESTINATION . COMPONENT Distribution)
else()
    install(TARGETS bg2e ${apps}
        RUNTIME_DEPENDENCY_SET bg2e_runtime
        RUNTIME DESTINATION . COMPONENT Distribution
        LIBRARY DESTINATION . COMPONENT Distribution)
    if(WIN32)
        # The project deliberately uses /MD in both Debug and Release.
        # Debug symbols do not imply a dependency on the MSVC debug CRT.
        set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP TRUE)
        set(CMAKE_INSTALL_DEBUG_LIBRARIES FALSE)
        include(InstallRequiredSystemLibraries)
        if(NOT CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS)
            message(FATAL_ERROR "MSVC redistributable runtime libraries were not found")
        endif()
        install(PROGRAMS ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS}
            DESTINATION . COMPONENT Distribution)
        set(runtime_dirs "${VULKAN_SDK}/Bin" "$<TARGET_FILE_DIR:bg2e>")
        foreach(runtime IN LISTS CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS)
            get_filename_component(runtime_dir "${runtime}" DIRECTORY)
            list(APPEND runtime_dirs "${runtime_dir}")
        endforeach()
        install(RUNTIME_DEPENDENCY_SET bg2e_runtime
            DIRECTORIES ${runtime_dirs}
            PRE_EXCLUDE_REGEXES "[Aa][Pp][Ii]-[Mm][Ss]-" "[Ee][Xx][Tt]-[Mm][Ss]-"
            POST_EXCLUDE_REGEXES ".*[/\\\\][Ss][Yy][Ss][Tt][Ee][Mm]32[/\\\\].*"
            RUNTIME DESTINATION . COMPONENT Distribution)
        foreach(target IN ITEMS bg2e ${apps})
            install(FILES "$<TARGET_PDB_FILE:${target}>" DESTINATION .
                CONFIGURATIONS Debug COMPONENT Distribution)
        endforeach()
    else()
        foreach(target IN ITEMS bg2e ${apps})
            set_target_properties(${target} PROPERTIES INSTALL_RPATH "$ORIGIN")
        endforeach()
        install(RUNTIME_DEPENDENCY_SET bg2e_runtime
            PRE_EXCLUDE_REGEXES "^linux-vdso" "^ld-linux" "^lib(c|m|pthread|dl|rt)\\.so"
            LIBRARY DESTINATION . COMPONENT Distribution)
    endif()
    # Include the built resources, including any app-specific shaders.
    list(GET apps 0 first_app)
    install(DIRECTORY "$<TARGET_FILE_DIR:${first_app}>/assets"
        "$<TARGET_FILE_DIR:${first_app}>/shaders"
        DESTINATION . COMPONENT Distribution)
endif()
