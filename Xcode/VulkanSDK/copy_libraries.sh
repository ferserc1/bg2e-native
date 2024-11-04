#!/bin/sh

mkdir -p ${PROJECT_DIR}/VulkanSDK/lib
cp ${VULKAN_SDK}/lib/libMoltenVK.dylib ${PROJECT_DIR}/VulkanSDK/lib
cp ${VULKAN_SDK}/lib/libvulkan.1.3.290.dylib ${PROJECT_DIR}/VulkanSDK/lib
cp ${VULKAN_SDK}/lib/libSDL2-2.0.0.dylib ${PROJECT_DIR}/VulkanSDK/lib

