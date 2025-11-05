
#include <bg2e/base/PlatformTools.hpp>
#include <string>
#include <filesystem>
#include <iostream>

#ifdef BG2E_IS_LINUX
#include <pwd.h>
#include <unistd.h>
#endif

#ifdef BG2E_IS_MAC

#include <CoreFoundation/CoreFoundation.h>
#include <pwd.h>
#include <unistd.h>

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

std::string bg2e_base_platform_tools_macos_app_preferences_path()
{
    CFURLRef appSupportURL = NULL;
    CFURLRef homeDirUrl = CFCopyHomeDirectoryURL();
    if (homeDirURL)
    {
        appSupportURL = CFURLCreateCopyAppendingPathComponent(
            kCFAllocatorDefault,
            homeDirURL,
            CFSTR("Library/Application Support"),
            true
        );
        CFReleasse(homeDirURL);
    }

    char c_path[2048] = {0};
    if (appSupportURL)
    {
        CFURLGetFileSystemRepresentation(
            appSupportURL,
            true,
            reinterpret_cast<UInt8*>(c_path),
            sizeof(c_path)
        );
        CFRelease(appSupportURL);
    }
    else
    {
        const char *nome = getenv("HOME");
        if (!home)
        {
            struct passwd * pw = getpwuid(getuid());
            home = pw->pw_dir;
        }
        snprintf(c_path, sizeof(c_path), "%s/Library/Application Support", home);
    }

    return std::string(c_path);
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

std::filesystem::path bg2e::base::PlatformTools::settingsPath(const std::string & appId)
{
    namespace fs = std::filesystem;
    std::filesystem::path basePath;
#ifdef BG2E_IS_MAC
    // TODO: Implement this
    basePath = bg2e_base_platform_tools_macos_app_preferences_path() / appId;
#elif BG2E_IS_LINUX
    const char * home = getenv("HOME");
    if (!home)
    {
        struct passwd * pw = getpwuid(getuid());
        home = pw->pw_dir;
    }
    basePath = fs::path(home) / ("." + appId);
#else
    // TODO: Implement this
    wchar_t path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, path)))
    {
        basePath = fs::path(path) / appId;
    }
    else
    {
        char *home = getenv("USERPROFILE");
        basePath = home ? fs::path(home) / appId : fs::current_path() / appId;
    }
#endif

    try {
        fs::create_directories(basePath);
    }
    catch (std::runtime_error err)
    {
        std::cerr << "Unable to create settings directory at path \"" << basePath << "\"" << std::endl; 
    }
    return basePath;
}
