if(NOT (UNIX AND NOT APPLE))
    return()
endif()

set(FFX_SC_TOOLS_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../FidelityFX-SDK-1.1.4/sdk/tools/ffx_shader_compiler")
set(FFX_SC_LIBS_PATH  "${FFX_SC_TOOLS_PATH}/libs")

find_program(GLSLANG_VALIDATOR
    NAMES glslangValidator glslang
    HINTS "${VULKAN_SDK}/bin" "$ENV{VULKAN_SDK}/bin"
    PATH_SUFFIXES bin
)
if(NOT GLSLANG_VALIDATOR)
    message(FATAL_ERROR "glslangValidator not found. Ensure VULKAN_SDK is set.")
endif()
message(STATUS "FidelityFX_SC_Linux: glslangValidator = ${GLSLANG_VALIDATOR}")

add_executable(ffx_sc_linux
    src/ffx_sc.cpp
    src/glsl_compiler.cpp
    "${FFX_SC_LIBS_PATH}/tiny-process-library/process.cpp"
    "${FFX_SC_LIBS_PATH}/tiny-process-library/process_unix.cpp"
    "${FFX_SC_LIBS_PATH}/SPIRV-Reflect/spirv_reflect.c"
    "${FFX_SC_LIBS_PATH}/MD5/md5.cpp"
)

target_include_directories(ffx_sc_linux PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${FFX_SC_LIBS_PATH}/tiny-process-library"
    "${FFX_SC_LIBS_PATH}/SPIRV-Reflect"
    "${FFX_SC_LIBS_PATH}/MD5"
)

target_compile_definitions(ffx_sc_linux PRIVATE
    FFX_LINUX_GLSLANG_DEFAULT="${GLSLANG_VALIDATOR}"
)

set_target_properties(ffx_sc_linux PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

target_link_libraries(ffx_sc_linux PRIVATE pthread)
