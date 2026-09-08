/*
 *    business grade graphic engine (bg2 engine)
 *    Copyright (C) 2026  Fernando Serrano Carpena
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <bg2e/base/PlatformTools.hpp>
#include <bg2e/app/MainLoop.hpp>

#include <string>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <system_error>
#include <vector>

#ifdef BG2E_IS_LINUX
#include <pwd.h>
#include <unistd.h>
#endif

#ifdef BG2E_IS_WINDOWS

#include <Windows.h>
#include <ShlObj_core.h>

#endif

bg2e::base::Platform bg2e::base::PlatformTools::currentPlatform()
{
#ifdef BG2E_IS_MAC
    return Platform::macOS;
#elif defined BG2E_IS_WINDOWS
    return Platform::Windows;
#else
    return Platform::Linux;
#endif
}

#ifndef BG2E_IS_MAC

std::filesystem::path bg2e::base::PlatformTools::applicationPath()
{
    namespace fs = std::filesystem;

#ifdef BG2E_IS_LINUX
    std::error_code error;
    auto executable = fs::read_symlink("/proc/self/exe", error);
    if (error)
    {
        throw std::system_error(error, "Unable to resolve /proc/self/exe");
    }
    if (!executable.is_absolute() || executable.parent_path().empty())
    {
        throw std::runtime_error("The resolved executable path is not absolute");
    }
    return executable.parent_path().lexically_normal();
#else
    std::vector<wchar_t> buffer(1024);

    while (true)
    {
        SetLastError(ERROR_SUCCESS);
        auto length = GetModuleFileNameW(
            nullptr,
            buffer.data(),
            static_cast<DWORD>(buffer.size())
        );

        if (length == 0)
        {
            throw std::system_error(
                static_cast<int>(GetLastError()),
                std::system_category(),
                "Unable to resolve the executable path"
            );
        }
        if (length < buffer.size())
        {
            auto executable = fs::path(std::wstring(buffer.data(), length));
            if (!executable.is_absolute() || executable.parent_path().empty())
            {
                throw std::runtime_error("The resolved executable path is not absolute");
            }
            return executable.parent_path().lexically_normal();
        }

        buffer.resize(buffer.size() * 2);
    }
#endif
}

#endif

std::filesystem::path bg2e::base::PlatformTools::shaderPath()
{
    auto basePath = applicationPath();
#ifdef BG2E_IS_MAC
    basePath /= "Contents";
    basePath /= "Resources";
#endif
    return basePath / "shaders";
}

std::filesystem::path bg2e::base::PlatformTools::assetPath()
{
    auto basePath = applicationPath();
#ifdef BG2E_IS_MAC
    basePath /= "Contents";
    basePath /= "Resources";
#endif
    return basePath / "assets";
}

// macOS version of settingsPath is defined in PlatformTools.mm
#ifndef BG2E_IS_MAC

std::filesystem::path bg2e::base::PlatformTools::settingsPath()
{
    namespace fs = std::filesystem;
    std::filesystem::path basePath;
    auto appId = app::MainLoop::current()->appId();

#ifdef BG2E_IS_LINUX
    const char * home = getenv("HOME");
    if (!home)
    {
        struct passwd * pw = getpwuid(getuid());
        home = pw->pw_dir;
    }
    basePath = fs::path(home) / ".config" / appId;
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
    catch (std::runtime_error const &)
    {
        std::cerr << "Unable to create settings directory at path \"" << basePath << "\"" << std::endl; 
    }
    return basePath;
}

std::filesystem::path bg2e::base::PlatformTools::homePath()
{
    namespace fs = std::filesystem;

#ifdef BG2E_IS_LINUX
    const char * home = getenv("HOME");
    if (!home)
    {
        struct passwd * pw = getpwuid(getuid());
        home = pw->pw_dir;
    }
    return fs::path(home);
#elif defined BG2E_IS_WINDOWS
    wchar_t path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_PROFILE, NULL, SHGFP_TYPE_CURRENT, path)))
    {
        return fs::path(path);
    }
    char * home = getenv("USERPROFILE");
    return home ? fs::path(home) : ".";
#else
#error "Unresolved platform"
#endif
}

#endif
