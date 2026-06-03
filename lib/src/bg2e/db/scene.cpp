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

#include <bg2e/db/scene.hpp>
#include <bg2e/json/JsonNode.hpp>

#include <fstream>

namespace bg2e::db {

bg2e::scene::Node * loadScene(
    const std::filesystem::path& filePath
) {
    return nullptr;
}

bg2e::scene::Node * loadScene(
    const std::filesystem::path& basePath,
    const std::string& fileName
) {
    return loadScene(basePath / fileName);
}

void saveScene(
    bg2e::scene::Node * sceneRoot,
    const std::filesystem::path& filePath
) {
    auto rootPath = filePath;
    rootPath.remove_filename();
    auto sceneData = sceneRoot->serialize(rootPath);
    
    std::ofstream file;
    file.open(filePath);
    if (file.is_open())
    {
        using namespace bg2e::json;
        auto sceneRoot = JSON(JsonObject{
            { "fileType", JSON("bg2e::scene") },
            { "version", JSON(JsonObject{
                { "major", JSON(1) },
                { "minor", JSON(0) },
                { "rev", JSON(0) },
            })},
            { "scene", JSON(JsonList{sceneData})}
        });
        file << sceneRoot->toString();
        file.close();
    }
}

void saveScene(
    bg2e::scene::Node * sceneRoot,
    const std::filesystem::path& basePath,
    const std::string& fileName
) {
    auto fullPath = basePath;
    fullPath += fileName;
    saveScene(sceneRoot, fullPath);
}

}
