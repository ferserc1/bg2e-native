#!/bin/sh

PLATFORM=$(uname)

# If linux and VULKAN_SDK is not set, use /usr
if [ "$PLATFORM" = "Linux" ]; then
    VULKAN_SDK=/usr
fi

# Check if the first and second arguments are set
if [ -z "$1" ] || [ -z "$2" ]; then
    echo "Usage: $0 <SHADERS_DIR> <OUT_DIR>"
    exit 1
fi

# Check if VULKAN_SDK is set and if the directory exists
if [ -z "$VULKAN_SDK" ] || [ ! -d "$VULKAN_SDK" ]; then
    echo "VULKAN_SDK is not set or the directory does not exist"
    exit 1
fi

GLSLANG=${VULKAN_SDK}/bin/glslang
mkdir -p $2
OUTPUT_DIR=$(cd $2; pwd)

if [ -f "${1}" ] && [[ "${1}" == *.glsl ]]; then
    FILE_NAME=$(basename ${1} .glsl)
    OUT_FILE=$FILE_NAME.spv
    echo "Compiling single shader file ${1} to ${OUTPUT_DIR}/${OUT_FILE}"
    ${GLSLANG} -V ${1} -o ${OUTPUT_DIR}/${OUT_FILE}
    exit 0
fi

INPUT_DIR=$(cd $1; pwd)

echo "Compiling shaders from ${INPUT_DIR} to ${OUTPUT_DIR}"

for path in ${INPUT_DIR}/*.glsl; do
    file_name=$(basename ${path} .glsl)
    echo "glslang" -V ${path} -o ${OUTPUT_DIR}/${file_name}.spv
    ${GLSLANG} -V ${path} -o ${OUTPUT_DIR}/${file_name}.spv
done
