#!/bin/sh

VK_SDK_PATH=$1

if [ -z "$1" ]
  then
    echo "Usage:"
    echo "./setup-deps.sh /path/to/VulkanSDK/1.3.xxx.x"
    exit -1
fi

if [ ! -d "$VK_SDK_PATH" ];
then
	echo "$VK_SDK_PATH does not exists"
    exit -2
fi

if [ ! -d "$VK_SDK_PATH/Applications" ] || [ ! -d "$VK_SDK_PATH/macOS/include" ] || [ ! -d "$VK_SDK_PATH/macOS/bin" ] || [ ! -d "$VK_SDK_PATH/MoltenVK" ];
then
	echo "\n\nERROR: The specified directory does not appear to be a valid VulkanSDK installation directory. Please, enter the path to the Vulkan version folder, for example:"
    echo "   % ./setup-deps.sh ~/VulkanSDK/1.3.239.0"
    echo "\nAlso, be sure to install VulkanSDK version 1.3 or higher, which can be obtained from the following URL:"
    echo "   https://vulkan.lunarg.com\n"
    exit -3
fi

# Symbolic link to VulkanSDK path:
if [ ! -s deps/VulkanSDK ];
then
    ln -s $VK_SDK_PATH deps/VulkanSDK
fi

# Check out bg2-io repository
if [ ! -d deps/bg2-io ];
then
    git clone https://github.com/ferserc1/bg2-io deps/bg2-io
fi

# Download GLFW binaries for macOS
if [ ! -d deps/glfw ];
then
    curl -L https://github.com/glfw/glfw/releases/download/3.3.8/glfw-3.3.8.bin.MACOS.zip --output deps/glfw.zip
    unzip deps/glfw.zip -d deps
    mv deps/glfw-3.3.8.bin.MACOS deps/glfw
    rm deps/glfw.zip
fi
