# Exports include directories and source files from third party dependencies
# - BG2E_THIRD_PARTY_INCLUDE_PATH
# - BG2E_THIRD_PARTY_SRC

set(THIRD_PARTY_PATH "${CMAKE_CURRENT_LIST_DIR}/../third_party")

# bg2io
file(GLOB BG2IO_SRC "${THIRD_PARTY_PATH}/bg2-io/*.c")
set(BG2IO_INCLUDE "${THIRD_PARTY_PATH}/bg2-io")

# bg2-scene
file(GLOB BG2SCENE_SRC "${THIRD_PARTY_PATH}/bg2-scene/*.cpp")
set(BG2SCENE_INCLUDE "${THIRD_PARTY_PATH}/bg2-scene")

# imgui
file(GLOB IMGUI_SRC "${THIRD_PARTY_PATH}/imgui/*.cpp")
set(IMGUI_INCLUDE "${THIRD_PARTY_PATH}/imgui")

# nativefiledialog
if(APPLE)
    set(NFD_SRC "${THIRD_PARTY_PATH}/nativefiledialog/src/nfd_cocoa.m")
    enable_language(OBJC)
    enable_language(OBJCXX)
    set_source_files_properties("${NFD_SRC}"
        PROPERTIES LANGUAGE OBJC
    )
elseif(UNIX AND NOT APPLE)
    set(NFD_SRC "${THIRD_PARTY_PATH}/nativefiledialog/src/nfd_gtk.cpp")
else()
    set(NFD_SRC "${THIRD_PARTY_PATH}/nativefiledialog/src/nfd_win.cpp")
endif()
set(NFD_INCLUDE "${THIRD_PARTY_PATH}/nativefiledialog/src/include")

# stb_image
set(STBIMAGE_INCLUDE "${THIRD_PARTY_PATH}/stb_image")

# tinyobj
set(TINYOBJ_INCLUDE "${THIRD_PARTY_PATH}/tinyobj")

# cgltf
set(CGLTF_INCLUDE "${THIRD_PARTY_PATH}/cgltf")

# metal-cpp (header-only, macOS only)
set(METALCPP_INCLUDE "${THIRD_PARTY_PATH}/metal-cpp")

# iryku_smaa (header-only, precomputed SMAA AreaTex/SearchTex data)
set(IRYKU_SMAA_INCLUDE "${THIRD_PARTY_PATH}/iryku_smaa")

# Export final variables
set(BG2E_THIRD_PARTY_INCLUDE_PATH
    "${BG2IO_INCLUDE}"
    "${BG2SCENE_INCLUDE}"
    "${IMGUI_INCLUDE}"
    "${NFD_INCLUDE}"
    "${STBIMAGE_INCLUDE}"
    "${TINYOBJ_INCLUDE}"
    "${CGLTF_INCLUDE}"
    "${METALCPP_INCLUDE}"
    "${IRYKU_SMAA_INCLUDE}"
    #"${SIMDJSON_INCLUDE}"
    #"${FASTGLTF_INCLUDE}"
    # Other dependency paths
)

set(BG2E_THIRD_PARTY_SRC
    ${BG2IO_SRC}
    ${BG2SCENE_SRC}
    ${IMGUI_SRC}
    ${NFD_SRC}
    #${SIMDJSON_SRC}
    #${FASTGLTF_SRC}
    # other dependency paths
)

# FidelityFX SDK (Linux and Windows only, not macOS)
if(NOT APPLE)
    if(UNIX AND NOT APPLE)
        # Build the Linux port of FidelityFX_SC before the SDK, so FFX_SC_EXECUTABLE resolves
        add_subdirectory("${THIRD_PARTY_PATH}/FidelityFX_SC_Linux" ffx_sc_linux_build)
        set(FFX_SC_EXECUTABLE "$<TARGET_FILE:ffx_sc_linux>" CACHE STRING "" FORCE)
    endif()

    set(FFX_API_BACKEND   "VK_X64")
    set(FFX_API_VK        ON)  # Required: not set by the SDK itself, checked on first line of vk/CMakeLists.txt
    set(FFX_FSR3          ON)  # Enables FSR3Upscaler, FrameInterpolation, OpticalFlow automatically
    set(FFX_BUILD_AS_DLL  OFF)
    set(FFX_AUTO_COMPILE_SHADERS ON CACHE BOOL "" FORCE)

    add_subdirectory("${THIRD_PARTY_PATH}/FidelityFX-SDK-1.1.4/sdk" ffx_sdk)

    if(UNIX AND NOT APPLE)
        # Ensure ffx_sc_linux is built before the shader permutation step runs
        add_dependencies(ffx_shader_permutations_vk ffx_sc_linux)
    endif()

    # FFX_PLATFORM_NAME is "x64" when FFX_API_BACKEND=VK_X64 (set by toolchain.cmake inside the SDK)
    set(BG2E_FFX_INCLUDE_PATH "${THIRD_PARTY_PATH}/FidelityFX-SDK-1.1.4/sdk/include")
    set(BG2E_FFX_LINK_LIBRARIES
        ffx_fsr3_x64
        ffx_backend_vk_x64
    )
endif()