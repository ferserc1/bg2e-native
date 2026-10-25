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

#pragma once

#include <bg2e/common.hpp>

#include <filesystem>
#include <string>

namespace bg2e::base {

// Owns one loader reference. Symbol pointers remain valid only while loaded.
class BG2E_API DynamicLibrary {
public:
    DynamicLibrary() noexcept = default;
    ~DynamicLibrary();
    DynamicLibrary(const DynamicLibrary&) = delete;
    DynamicLibrary& operator=(const DynamicLibrary&) = delete;
    DynamicLibrary(DynamicLibrary&& other) noexcept;
    DynamicLibrary& operator=(DynamicLibrary&& other) noexcept;

    // Throws std::runtime_error (POSIX) or std::system_error (Windows) on failure.
    static DynamicLibrary load(const std::filesystem::path& path);
    void unload() noexcept;

    // Throws if unloaded or the symbol cannot be resolved.
    void* getSymbol(const std::string& name) const;
    bool isLoaded() const noexcept;

private:
    explicit DynamicLibrary(void* handle) noexcept;

    void* _handle = nullptr;
};

}
