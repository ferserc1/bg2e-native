#!/bin/bash

# This script packages Vulkan resources into a macOS app bundle.
if [ "$#" -lt 2 ]; then
    echo
    echo "Usage: $0 <app-bundle-path.app> <signing-identity>"
    echo
    exit 1
fi

# Check if VULKAN_SDK is defined
if [ -z "$VULKAN_SDK" ]; then
    echo
    echo "Error: The VULKAN_SDK environment variable is not defined."
    echo "Please run the setup-env.sh script inside the VulkanSDK directory to configure it."
    echo "Example: source ~/VulkanSDK/1.x.xxx.x/setup-env.sh"
    echo
    exit 1
fi

APP_PACKAGE_PATH="$1"
SIGNING_IDENTITY="$2"
echo
echo "Packaging Vulkan resources for the app bundle '$APP_PACKAGE_PATH'..."
echo "Using signing identity: $SIGNING_IDENTITY"

VULKAN_SHARE_PATH="$VULKAN_SDK/share/vulkan"
echo "  Vulkan share path: $VULKAN_SHARE_PATH"

VULKAN_VERSION=$(basename "$(dirname "$VULKAN_SDK")")
echo "  Vulkan version: $VULKAN_VERSION"

LIBVULKAN_VERSION=$(echo "$VULKAN_VERSION" | awk -F. '{OFS="."; $NF=""; print $0}' | sed 's/\.$//')
LIBVULKAN_NAME="libvulkan.$LIBVULKAN_VERSION.dylib"
echo "  Libvulkan name: $LIBVULKAN_NAME"

RESOURCES_DIR="$APP_PACKAGE_PATH/Contents/Resources"
FRAMEWORKS_DIR="$APP_PACKAGE_PATH/Contents/Frameworks"
LIB_DIR="$APP_PACKAGE_PATH/Contents/lib"

FRAMEWORKS_FILES=("libSDL2-2.0.0.dylib" "$LIBVULKAN_NAME")
echo "  Frameworks files: ${FRAMEWORKS_FILES[@]}"

LIB_FILES=("libMoltenVK.dylib" "libVkLayer_khronos_validation.dylib")
echo "  Lib files: ${LIB_FILES[@]}"

# Ensure destination directories exist
mkdir -p "$FRAMEWORKS_DIR"
mkdir -p "$LIB_DIR"
mkdir -p "$RESOURCES_DIR/vulkan"

cp -R "$VULKAN_SHARE_PATH/icd.d" "$RESOURCES_DIR/vulkan"
cp -R "$VULKAN_SHARE_PATH/explicit_layer.d" "$RESOURCES_DIR/vulkan"

# Copy frameworks to the Frameworks directory and sign them
for framework in "${FRAMEWORKS_FILES[@]}"; do
    cp "$VULKAN_SDK/lib/$framework" "$FRAMEWORKS_DIR/"
    echo "  - Copied $framework to $FRAMEWORKS_DIR"
    codesign --deep --force --sign "$SIGNING_IDENTITY" "$FRAMEWORKS_DIR/$framework"
    echo "  - Signed $framework with identity $SIGNING_IDENTITY"
done

# Copy lib files to the lib directory and sign them
for lib_file in "${LIB_FILES[@]}"; do
    cp "$VULKAN_SDK/lib/$lib_file" "$LIB_DIR/"
    echo "  - Copied $lib_file to $LIB_DIR"
    codesign --deep --force --sign "$SIGNING_IDENTITY" "$LIB_DIR/$lib_file"
    echo "  - Signed $lib_file with identity $SIGNING_IDENTITY"
done

# Create symbolic link for libvulkan.1.dylib
ln -sf "$FRAMEWORKS_DIR/$LIBVULKAN_NAME" "$FRAMEWORKS_DIR/libvulkan.1.dylib"
echo "  - Created symbolic link libvulkan.1.dylib -> $LIBVULKAN_NAME"


