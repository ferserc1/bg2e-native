#!/bin/sh

mkdir -p ${PROJECT_DIR}/VulkanSDK/lib
DEPS=(
    "libMoltenVK.dylib"
    "libSDL2-2.0.0.dylib"
    "libVkLayer_khronos_validation.dylib"
    "libvulkan.1.*.dylib"
)
for dep in ${DEPS[@]}; do
    echo "Copy ${dep}..."
    cp "${VULKAN_SDK}/lib/"$dep "${PROJECT_DIR}/VulkanSDK/lib"
done
