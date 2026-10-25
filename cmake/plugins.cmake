include_guard(GLOBAL)

# TARGET_FILE_DIR/TARGET_FILE_NAME in deployment commands must not introduce an
# implicit dependency back to the app which itself depends on deployment.
cmake_policy(PUSH)
if(POLICY CMP0112)
    cmake_policy(SET CMP0112 NEW)
endif()
set(BG2E_PLUGIN_CMAKE_DIR "${CMAKE_CURRENT_LIST_DIR}" CACHE INTERNAL "Shared plugin helper directory" FORCE)

# Explicit additional ABI-sensitive compiler options. Standard compiler/library/
# runtime/configuration macros are fingerprinted independently in Sdk.hpp.
function(bg2e_configure_plugin_sdk ENGINE_TARGET)
    set(BG2E_PLUGIN_ABI_OPTIONS "" CACHE STRING "ABI-sensitive options shared by engine and plugins")
    get_target_property(BG2E_SDK_MSVC_RUNTIME ${ENGINE_TARGET} MSVC_RUNTIME_LIBRARY)
    if(NOT BG2E_SDK_MSVC_RUNTIME)
        if(MSVC)
            message(FATAL_ERROR "Declare the engine's MSVC_RUNTIME_LIBRARY before configuring its SDK")
        endif()
        set(BG2E_SDK_MSVC_RUNTIME "")
    endif()
    string(SHA256 BG2E_ABI_SIGNATURE "${BG2E_PLUGIN_ABI_OPTIONS}")
    set(SDK_DIR "${PRODUCT_DIR}/sdk")
    file(MAKE_DIRECTORY "${SDK_DIR}/include/bg2e/plugin")
    configure_file("${BG2E_PLUGIN_CMAKE_DIR}/PluginBuildConfig.hpp.in"
        "${SDK_DIR}/include/bg2e/plugin/BuildConfig.hpp" @ONLY)
    configure_file("${BG2E_PLUGIN_CMAKE_DIR}/PluginSdk.cmake.in"
        "${SDK_DIR}/bg2e-sdk.cmake" @ONLY)
    target_include_directories(${ENGINE_TARGET} BEFORE PUBLIC "${SDK_DIR}/include")
    target_compile_options(${ENGINE_TARGET} PUBLIC ${BG2E_PLUGIN_ABI_OPTIONS})
    set_target_properties(${ENGINE_TARGET} PROPERTIES
        BG2E_SDK_INCLUDE_DIRECTORY "${SDK_DIR}/include"
        BG2E_PLUGIN_ABI_OPTIONS "${BG2E_PLUGIN_ABI_OPTIONS}")
endfunction()

# Always refresh the application's one engine copy, including library-only edits.
# Non-Apple apps sharing a runtime directory also share one deployment writer.
function(bg2e_stage_engine_runtime)
    cmake_parse_arguments(RT "" "APP_TARGET;ENGINE_TARGET" "" ${ARGN})
    if(NOT TARGET "${RT_APP_TARGET}" OR NOT TARGET "${RT_ENGINE_TARGET}")
        message(FATAL_ERROR "bg2e_stage_engine_runtime requires existing APP_TARGET and ENGINE_TARGET")
    endif()
    get_target_property(SDK_INCLUDE ${RT_ENGINE_TARGET} BG2E_SDK_INCLUDE_DIRECTORY)
    if(SDK_INCLUDE)
        target_include_directories(${RT_APP_TARGET} BEFORE PRIVATE "${SDK_INCLUDE}")
    endif()
    if(MSVC)
        get_target_property(ENGINE_RUNTIME ${RT_ENGINE_TARGET} MSVC_RUNTIME_LIBRARY)
        if(ENGINE_RUNTIME)
            set_property(TARGET ${RT_APP_TARGET} PROPERTY MSVC_RUNTIME_LIBRARY "${ENGINE_RUNTIME}")
        endif()
    endif()
    get_target_property(OUTPUT_DIR ${RT_APP_TARGET} RUNTIME_OUTPUT_DIRECTORY)
    set(KEY "${OUTPUT_DIR}|${RT_ENGINE_TARGET}|${VULKAN_SDK}")
    if(NOT OUTPUT_DIR)
        string(APPEND KEY "|${RT_APP_TARGET}")
    endif()
    foreach(CONFIG IN LISTS CMAKE_CONFIGURATION_TYPES)
        string(TOUPPER "${CONFIG}" UPPER)
        get_target_property(CONFIG_DIR ${RT_APP_TARGET} RUNTIME_OUTPUT_DIRECTORY_${UPPER})
        string(APPEND KEY "|${CONFIG}=${CONFIG_DIR}")
    endforeach()
    if(APPLE)
        string(APPEND KEY "|${RT_APP_TARGET}")
        set(DEST "$<TARGET_FILE_DIR:${RT_APP_TARGET}>/../lib")
    else()
        set(DEST "$<TARGET_FILE_DIR:${RT_APP_TARGET}>")
    endif()
    string(SHA256 HASH "${KEY}")
    set(STAGE_TARGET "bg2e_runtime_${HASH}")
    if(NOT TARGET ${STAGE_TARGET})
        get_filename_component(TOOL_DIR "${CMAKE_LINKER}" DIRECTORY)
        add_custom_target(${STAGE_TARGET}
            COMMAND ${CMAKE_COMMAND}
                "-DENGINE_FILE=$<TARGET_FILE:${RT_ENGINE_TARGET}>"
                "-DDESTINATION=${DEST}"
                "-DSDK_ROOT=${VULKAN_SDK}"
                "-DSDL_LIBRARY=${SDL2_LIBRARY}"
                "-DTOOL_DIR=${TOOL_DIR}"
                -P "${BG2E_PLUGIN_CMAKE_DIR}/stage_engine_runtime.cmake"
            DEPENDS ${RT_ENGINE_TARGET}
            VERBATIM)
    endif()
    add_dependencies(${RT_APP_TARGET} ${STAGE_TARGET})
    if(APPLE)
        set_target_properties(${RT_APP_TARGET} PROPERTIES
            INSTALL_RPATH "@executable_path/../lib" BUILD_WITH_INSTALL_RPATH TRUE)
    elseif(UNIX)
        set_target_properties(${RT_APP_TARGET} PROPERTIES
            INSTALL_RPATH "$ORIGIN" BUILD_WITH_INSTALL_RPATH TRUE)
    endif()
endfunction()

# bg2e_add_plugin(TARGET_NAME module APP_TARGET app SOURCES ...
#     [ENGINE_TARGET shared_engine] [INCLUDE_DIRECTORIES ...])
# Works unchanged with the repository engine and the imported standalone engine.
function(bg2e_add_plugin)
    cmake_parse_arguments(P "" "TARGET_NAME;APP_TARGET;ENGINE_TARGET" "SOURCES;INCLUDE_DIRECTORIES" ${ARGN})
    if(P_UNPARSED_ARGUMENTS OR NOT P_TARGET_NAME OR NOT P_APP_TARGET OR NOT P_SOURCES)
        message(FATAL_ERROR "bg2e_add_plugin requires TARGET_NAME, APP_TARGET and SOURCES; see cmake/PLUGINS.md")
    endif()
    if(NOT P_ENGINE_TARGET)
        if(BG2E_LIB_TARGET)
            set(P_ENGINE_TARGET "${BG2E_LIB_TARGET}")
        else()
            set(P_ENGINE_TARGET bg2e)
        endif()
    endif()
    if(NOT TARGET "${P_APP_TARGET}" OR NOT TARGET "${P_ENGINE_TARGET}")
        message(FATAL_ERROR "Create the application and shared engine targets before bg2e_add_plugin")
    endif()
    get_target_property(ENGINE_TYPE ${P_ENGINE_TARGET} TYPE)
    if(NOT ENGINE_TYPE STREQUAL "SHARED_LIBRARY")
        message(FATAL_ERROR "Plugins must link the application's SHARED engine, not a static duplicate")
    endif()
    bg2e_stage_engine_runtime(APP_TARGET ${P_APP_TARGET} ENGINE_TARGET ${P_ENGINE_TARGET})
    get_target_property(SDK_INCLUDE ${P_ENGINE_TARGET} BG2E_SDK_INCLUDE_DIRECTORY)
    get_target_property(ENGINE_RUNTIME ${P_ENGINE_TARGET} MSVC_RUNTIME_LIBRARY)
    if(NOT SDK_INCLUDE OR NOT EXISTS "${SDK_INCLUDE}/bg2e/plugin/BuildConfig.hpp")
        message(FATAL_ERROR "Engine SDK build profile is missing; regenerate/update the paired engine SDK")
    endif()
    find_path(BG2E_GLM_INCLUDE_DIR glm/glm.hpp
        HINTS ${VULKAN_INCLUDE_PATH} "${VULKAN_SDK}/include" "${VULKAN_SDK}/Include")
    if(NOT BG2E_GLM_INCLUDE_DIR)
        message(FATAL_ERROR "The public plugin API requires GLM headers (glm/glm.hpp)")
    endif()
    add_library(${P_TARGET_NAME} MODULE ${P_SOURCES})
    target_compile_features(${P_TARGET_NAME} PRIVATE cxx_std_20)
    target_compile_definitions(${P_TARGET_NAME} PRIVATE BG2E_PLUGIN_BUILD)
    target_include_directories(${P_TARGET_NAME} BEFORE PRIVATE "${SDK_INCLUDE}")
    target_include_directories(${P_TARGET_NAME} PRIVATE
        ${BG2E_INCLUDE_PATH} ${VULKAN_INCLUDE_PATH} ${SDL2_INCLUDE_DIRS}
        ${BG2E_GLM_INCLUDE_DIR} ${P_INCLUDE_DIRECTORIES})
    target_link_libraries(${P_TARGET_NAME} PRIVATE ${P_ENGINE_TARGET})
    set_target_properties(${P_TARGET_NAME} PROPERTIES
        CXX_STANDARD 20 CXX_STANDARD_REQUIRED YES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/plugins/${P_TARGET_NAME}"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/plugins/${P_TARGET_NAME}"
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/plugins/${P_TARGET_NAME}")
    if(MSVC)
        if(NOT ENGINE_RUNTIME)
            message(FATAL_ERROR "The shared engine must declare its MSVC_RUNTIME_LIBRARY")
        endif()
        set_property(TARGET ${P_TARGET_NAME} PROPERTY MSVC_RUNTIME_LIBRARY "${ENGINE_RUNTIME}")
    endif()
    if(APPLE)
        set(DEST "$<TARGET_FILE_DIR:${P_APP_TARGET}>/../Resources/plugins")
        set_target_properties(${P_TARGET_NAME} PROPERTIES
            INSTALL_RPATH "@loader_path/../../lib;@executable_path/../lib"
            BUILD_WITH_INSTALL_RPATH TRUE)
    else()
        set(DEST "$<TARGET_FILE_DIR:${P_APP_TARGET}>/plugins")
        if(UNIX)
            set_target_properties(${P_TARGET_NAME} PROPERTIES
                INSTALL_RPATH "$ORIGIN/.." BUILD_WITH_INSTALL_RPATH TRUE)
        endif()
    endif()
    add_custom_target(${P_TARGET_NAME}_deploy
        COMMAND ${CMAKE_COMMAND} -E make_directory "${DEST}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:${P_TARGET_NAME}>" "${DEST}/$<TARGET_FILE_NAME:${P_TARGET_NAME}>"
        DEPENDS ${P_TARGET_NAME}
        VERBATIM)
    add_dependencies(${P_APP_TARGET} ${P_TARGET_NAME}_deploy)
endfunction()
cmake_policy(POP)
