
#include <bg2e/base/PlatformTools.hpp>

#ifdef BG2E_IS_MAC

#include <CoreFoundation/CoreFoundation.h>

std::string bg2e_base_platform_tools_macos_bundle_path()
{
    auto appBundle = CFBundleGetMainBundle();
    CFURLRef appUrlRef = CFBundleCopyBundleURL(appBundle);
    char c_path[2048] = { '\0' };

    CFURLGetFileSystemRepresentation(appUrlRef, true, reinterpret_cast<UInt8*>(c_path), 2048);

    CFRelease(appUrlRef);

    return std::string(c_path) + "/";
}

std::string bg2e_base_platform_tools_macos_resources_path()
{
    auto appBundle = CFBundleGetMainBundle();
    auto resourcesUrl = CFBundleCopyResourcesDirectoryURL(appBundle);
    char c_path[2048] = { '\0' };

    CFURLGetFileSystemRepresentation(resourcesUrl, true, reinterpret_cast<UInt8*>(c_path), 2048);

    CFRelease(resourcesUrl);

    return std::string(c_path) + "/";
}

#endif

std::filesystem::path bg2e::base::PlatformTools::shaderPath()
{
#ifdef BG2E_IS_MAC
    return bg2e_base_platform_tools_macos_resources_path() + "shaders/";
#else
    return "shaders/";
#endif
}

std::filesystem::path bg2e::base::PlatformTools::assetPath()
{
#ifdef BG2E_IS_MAC
    return bg2e_base_platform_tools_macos_resources_path() + "assets/";
#else
    return "assets/";
#endif
}
