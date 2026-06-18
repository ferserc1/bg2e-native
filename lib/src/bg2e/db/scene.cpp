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
#include <bg2e/scene/Node.hpp>
#include <bg2e/scene/Scene.hpp>
#include <bg2e/scene/DrawableRegistry.hpp>
#include <bg2e/scene/FindNodeComponentVisitor.hpp>
#include <bg2e/scene/DrawableComponent.hpp>
#include <bg2e/json/JsonNode.hpp>
#include <bg2e/json/JsonParser.hpp>
#include <bg2e/base/Log.hpp>

#include <fstream>

namespace bg2e::db {

static int countJsonNodes(std::shared_ptr<bg2e::json::JsonNode> jsonNode)
{
    if (!jsonNode || !jsonNode->isObject()) { return 0; }
    auto& obj = jsonNode->objectValue();
    int count = 1;
    if (obj.count("children") && obj["children"]->isList())
    {
        for (auto& child : obj["children"]->listValue())
        {
            count += countJsonNodes(child);
        }
    }
    return count;
}

static int countSceneNodes(bg2e::scene::Node* node)
{
    if (!node) { return 0; }
    int count = 1;
    for (auto& child : node->children())
    {
        count += countSceneNodes(child.get());
    }
    return count;
}

std::shared_ptr<bg2e::scene::Scene> loadScene(
    const std::filesystem::path& filePath,
    bg2e::render::Engine& engine,
    bg2e::scene::SceneProgressCallback onProgress
) {
    std::ifstream inFile(filePath);
    if (!inFile.is_open())
    {
        bg2e_log_error << "Could not open scene file at path \"" << filePath << "\""  << bg2e_log_end;
        return nullptr;
    }

    inFile.seekg(0, std::ios::end);
    std::string content;
    content.resize(inFile.tellg());

    inFile.seekg(0, std::ios::beg);
    inFile.read(&content[0], content.size());

    auto parser = json::JsonParser(content);
    auto sceneFile = parser.parse();

    if (!sceneFile) {
        bg2e_log_error << "Error parsing scene file at path \"" << filePath << "\"" << bg2e_log_end;
        return nullptr;
    }

    // Build progress context and pre-count nodes
    scene::SceneLoadProgress progress;
    if (onProgress)
    {
        progress.callback = onProgress;
        auto& obj = sceneFile->objectValue();
        if (obj.count("scene") && obj["scene"]->isList())
        {
            for (auto& nodeData : obj["scene"]->listValue())
            {
                progress.total += countJsonNodes(nodeData);
            }
        }
    }

    auto scene = scene::Scene::deserialize(sceneFile, filePath.parent_path(), engine, onProgress ? &progress : nullptr);
    inFile.close();

    if (!scene)
    {
        bg2e_log_error << "Error loading scene content from file \"" << filePath << "\". This error is usually caused by issues with the scene configuration or missing resources." << bg2e_log_end;
    }

    return scene;
}

std::shared_ptr<bg2e::scene::Scene> loadScene(
    const std::filesystem::path& basePath,
    const std::string& fileName,
    bg2e::render::Engine& engine,
    bg2e::scene::SceneProgressCallback onProgress
) {
    return loadScene(basePath / fileName, engine, onProgress);
}

void saveScene(
    bg2e::scene::Node* sceneRoot,
    const std::filesystem::path& filePath,
    bg2e::scene::SceneProgressCallback onProgress
) {
    auto rootPath = filePath;
    rootPath.remove_filename();

    bg2e::scene::DrawableRegistry registry;
    bg2e::scene::FindNodeComponentVisitor<bg2e::scene::DrawableComponent> findDrawables;
    auto drawableNodes = findDrawables.find(sceneRoot);
    for (auto& weakNode : drawableNodes)
    {
        if (auto node = weakNode.lock())
        {
            auto comp = node->getComponent<bg2e::scene::DrawableComponent>();
            if (comp && comp->drawableBase())
            {
                registry.registerDrawable(comp->drawableBase());
            }
        }
    }

    // Build progress context and pre-count nodes
    scene::SceneSaveProgress progress;
    if (onProgress)
    {
        progress.callback = onProgress;
        progress.total = countSceneNodes(sceneRoot);
    }

    auto sceneData = sceneRoot->serialize(rootPath, onProgress ? &progress : nullptr);

    std::ofstream file;
    file.open(filePath);
    if (file.is_open())
    {
        using namespace bg2e::json;
        auto sceneJson = JSON(JsonObject{
            { "fileType", JSON("bg2e::scene") },
            { "version", JSON(JsonObject{
                { "major", JSON(1) },
                { "minor", JSON(0) },
                { "rev", JSON(0) },
            })},
            { "scene", JSON(JsonList{sceneData})}
        });
        file << sceneJson->toString();
        file.close();
    }

    registry.cleanup();
}

void saveScene(
    bg2e::scene::Node* sceneRoot,
    const std::filesystem::path& basePath,
    const std::string& fileName,
    bg2e::scene::SceneProgressCallback onProgress
) {
    saveScene(sceneRoot, basePath / fileName, onProgress);
}

}
