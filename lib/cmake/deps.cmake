# Exports include directories and source files from third party dependencies
# - BG2E_THIRD_PARTY_INCLUDE_PATH
# - BG2E_THIRD_PARTY_SRC

set(THIRD_PARTY_PATH "${CMAKE_CURRENT_LIST_DIR}/../third_party")
set(BG2IO_PATH "${CMAKE_CURRENT_LIST_DIR}/../bg2-io")

# bg2io
file(GLOB BG2IO_SRC "${BG2IO_PATH}/src/bg2-io/*.c")
set(BG2IO_INCLUDE "${BG2IO_PATH}/src/bg2-io")

# bg2-scene
file(GLOB BG2SCENE_SRC "${BG2IO_PATH}/src/bg2-scene/*.cpp")
set(BG2SCENE_INCLUDE "${BG2IO_PATH}/src/bg2-scene")

# imgui
file(GLOB IMGUI_SRC "${THIRD_PARTY_PATH}/imgui/*.cpp")
set(IMGUI_INCLUDE "${THIRD_PARTY_PATH}/imgui")

# nativefiledialog-extended
if(APPLE)
    set(NFD_SRC "${THIRD_PARTY_PATH}/nativefiledialog-extended/src/nfd_cocoa.m")
    enable_language(OBJC)
    enable_language(OBJCXX)
    set_source_files_properties("${NFD_SRC}"
        PROPERTIES LANGUAGE OBJC
    )
elseif(UNIX AND NOT APPLE)
    set(NFD_SRC "${THIRD_PARTY_PATH}/nativefiledialog-extended/src/nfd_gtk.cpp")
else()
    set(NFD_SRC "${THIRD_PARTY_PATH}/nativefiledialog-extended/src/nfd_win.cpp")
endif()
set(NFD_INCLUDE "${THIRD_PARTY_PATH}/nativefiledialog-extended/src/include")

# stb_image
set(STBIMAGE_INCLUDE "${THIRD_PARTY_PATH}/stb_image")

# tinyobj
set(TINYOBJ_INCLUDE "${THIRD_PARTY_PATH}/tinyobj")

# fastgltf has become an extremely complex, verbose library that changes between versions, which is why it has been abandoned in favor of cgltf.
# simdjson
# set(SIMDJSON_SRC "${THIRD_PARTY_PATH}/simdjson/simdjson.cpp")
#set(SIMDJSON_INCLUDE "${THIRD_PARTY_PATH}/simdjson")
# fastgltf
# file(GLOB FASTGLTF_SRC "${THIRD_PARTY_PATH}/fastgltf/src/*.cpp")
# set(FASTGLTF_INCLUDE "${THIRD_PARTY_PATH}/fastgltf/include")

# cgltf
set(CGLTF_INCLUDE "${THIRD_PARTY_PATH}/cgltf")

message("FastGLTF" "Includes: ${FASTGLTF_INCLUDE}")

# Export final variables
set(BG2E_THIRD_PARTY_INCLUDE_PATH
    "${BG2IO_INCLUDE}"
    "${BG2SCENE_INCLUDE}"
    "${IMGUI_INCLUDE}"
    "${NFD_INCLUDE}"
    "${STBIMAGE_INCLUDE}"
    "${TINYOBJ_INCLUDE}"
    "${CGLTF_INCLUDE}"
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