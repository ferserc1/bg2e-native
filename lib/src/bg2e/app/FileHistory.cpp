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

#include <bg2e/app/FileHistory.hpp>

#include <algorithm>
#include <stdexcept>

namespace bg2e::app {

const std::string FileHistory::Image = "image";
const std::string FileHistory::Bg2Model = "bg2model";
const std::string FileHistory::Model3D = "model3d";

FileHistory * FileHistory::g_singleton = nullptr;

FileHistory& FileHistory::get()
{
    if (g_singleton == nullptr)
    {
        g_singleton = new FileHistory();
    }
    return *g_singleton;
}

FileHistory::FileHistory()
{
    registerType(Image, "Images", { "jpg", "jpeg", "png", "bmp", "webp" });
    registerType(Bg2Model, "bg2e model", { "bg2" });
    registerType(Model3D, "3D models", { "bg2", "obj", "gltf", "glb" });
}

void FileHistory::registerType(const std::string& typeName,
                               const std::string& filterLabel,
                               const std::vector<std::string>& extensions)
{
    std::string extensionList;
    for (size_t i = 0; i < extensions.size(); ++i)
    {
        if (i > 0)
        {
            extensionList += ",";
        }
        extensionList += extensions[i];
    }

    _typeFilters[typeName] = FileDialog::FileFilters { { filterLabel, extensionList } };
    _histories.try_emplace(typeName);
}

bool FileHistory::isTypeRegistered(const std::string& typeName) const
{
    return _typeFilters.find(typeName) != _typeFilters.end();
}

const FileDialog::FileFilters& FileHistory::filtersFor(const std::string& typeName) const
{
    auto it = _typeFilters.find(typeName);
    if (it == _typeFilters.end())
    {
        throw std::runtime_error("FileHistory::filtersFor(): unregistered type '" + typeName + "'");
    }
    return it->second;
}

void FileHistory::add(const std::string& typeName, const std::filesystem::path& path)
{
    if (!isTypeRegistered(typeName))
    {
        throw std::runtime_error("FileHistory::add(): unregistered type '" + typeName + "'");
    }

    auto & list = _histories[typeName];
    // Copy before mutating: path may reference an element of list itself
    // (e.g. a caller iterating entries()), so erase/insert would invalidate it
    auto newPath = path;
    list.erase(std::remove(list.begin(), list.end(), newPath), list.end());
    list.insert(list.begin(), std::move(newPath));
    if (list.size() > MaxEntriesPerType)
    {
        list.resize(MaxEntriesPerType);
    }
}

const std::vector<std::filesystem::path>& FileHistory::entries(const std::string& typeName) const
{
    static const std::vector<std::filesystem::path> emptyEntries;
    auto it = _histories.find(typeName);
    return it != _histories.end() ? it->second : emptyEntries;
}

bool FileHistory::empty(const std::string& typeName) const
{
    return entries(typeName).empty();
}

void FileHistory::clear(const std::string& typeName)
{
    auto it = _histories.find(typeName);
    if (it != _histories.end())
    {
        it->second.clear();
    }
}

void FileHistory::clearAll()
{
    for (auto & [typeName, history] : _histories)
    {
        history.clear();
    }
}

}
