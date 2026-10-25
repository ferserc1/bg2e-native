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

#include <bg2e/base/DynamicLibrary.hpp>

#include <stdexcept>
#include <system_error>
#include <utility>

#ifdef _WIN32
#include <Windows.h>
#else
#include <dlfcn.h>
#endif

namespace bg2e::base {

DynamicLibrary::DynamicLibrary(void* handle) noexcept
    : _handle(handle)
{
}

DynamicLibrary::~DynamicLibrary()
{
    unload();
}

DynamicLibrary::DynamicLibrary(DynamicLibrary&& other) noexcept
    : _handle(std::exchange(other._handle, nullptr))
{
}

DynamicLibrary& DynamicLibrary::operator=(DynamicLibrary&& other) noexcept
{
    if (this != &other)
    {
        unload();
        _handle = std::exchange(other._handle, nullptr);
    }
    return *this;
}

DynamicLibrary DynamicLibrary::load(const std::filesystem::path& path)
{
    // An absolute path avoids platform-specific loader search rules for bare names.
    const auto absolutePath = std::filesystem::absolute(path);
#ifdef _WIN32
    auto handle = LoadLibraryW(absolutePath.c_str());
    if (!handle)
    {
        throw std::system_error(static_cast<int>(GetLastError()), std::system_category(),
                                "Unable to load dynamic library");
    }
    return DynamicLibrary(static_cast<void*>(handle));
#else
    auto handle = dlopen(absolutePath.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (!handle)
    {
        const char* error = dlerror();
        throw std::runtime_error(std::string("Unable to load dynamic library: ") +
                                 (error ? error : "Unknown loader error"));
    }
    return DynamicLibrary(handle);
#endif
}

void DynamicLibrary::unload() noexcept
{
    if (auto handle = std::exchange(_handle, nullptr))
    {
#ifdef _WIN32
        FreeLibrary(static_cast<HMODULE>(handle));
#else
        dlclose(handle);
#endif
    }
}

void* DynamicLibrary::getSymbol(const std::string& name) const
{
    if (!isLoaded())
    {
        throw std::logic_error("Cannot resolve a symbol from an unloaded dynamic library");
    }
    if (name.empty() || name.find('\0') != std::string::npos)
    {
        throw std::invalid_argument("A symbol name must be nonempty and contain no null characters");
    }
#ifdef _WIN32
    auto symbol = GetProcAddress(static_cast<HMODULE>(_handle), name.c_str());
    if (!symbol)
    {
        throw std::system_error(static_cast<int>(GetLastError()), std::system_category(),
                                "Unable to resolve symbol '" + name + "'");
    }
    return reinterpret_cast<void*>(symbol);
#else
    dlerror();
    auto symbol = dlsym(_handle, name.c_str());
    if (const char* error = dlerror())
    {
        throw std::runtime_error("Unable to resolve symbol '" + name + "': " + error);
    }
    return symbol;
#endif
}

bool DynamicLibrary::isLoaded() const noexcept
{
    return _handle != nullptr;
}

}
