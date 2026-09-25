/*
 *    business grade graphic engine (bg2 engine)
 *    Copyright (C) 2026  Fernando Serrano Carpena
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation: either version 3 of the License, or
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
#include <bg2e/app/FileDialog.hpp>

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

namespace bg2e {
namespace app {

class BG2E_API FileHistory {
public:
    static FileHistory& get();

    // Well-known type names (registered automatically in the constructor)
    static const std::string Image;
    static const std::string Bg2Model;
    static const std::string Model3D;

    // Type registry. Extensions are specified without a leading dot.
    void registerType(const std::string& typeName,
                      const std::string& filterLabel,
                      const std::vector<std::string>& extensions);
    bool isTypeRegistered(const std::string& typeName) const;
    const FileDialog::FileFilters& filtersFor(const std::string& typeName) const;

    // History is ordered from most recently selected to least recently selected.
    void add(const std::string& typeName, const std::filesystem::path& path);
    const std::vector<std::filesystem::path>& entries(const std::string& typeName) const;
    bool empty(const std::string& typeName) const;
    void clear(const std::string& typeName);
    void clearAll();

    static constexpr size_t MaxEntriesPerType = 32;

protected:
    FileHistory();
    virtual ~FileHistory() = default;

    static FileHistory * g_singleton;

    std::unordered_map<std::string, FileDialog::FileFilters> _typeFilters;
    std::unordered_map<std::string, std::vector<std::filesystem::path>> _histories;
};

}
}
