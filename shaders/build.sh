#!/bin/sh

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
INPUT_DIR=$(cd $1; pwd)
mkdir -p $2
OUTPUT_DIR=$(cd $2; pwd)

echo "Compiling shaders from ${INPUT_DIR} to ${OUTPUT_DIR}"

for path in ${INPUT_DIR}/*.glsl; do
    file_name=$(basename ${path} .glsl)
    echo ${GLSLANG} -V ${path} -o ${OUTPUT_DIR}/${file_name}.spv
    ${GLSLANG} -V ${path} -o ${OUTPUT_DIR}/${file_name}.spv
done
