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
#include <bg2e/db/mesh_bg2.hpp>
#include <bg2e/scene/Node.hpp>
#include <bg2e/scene/Scene.hpp>
#include <bg2e/scene/DrawableRegistry.hpp>
#include <bg2e/scene/FindNodeComponentVisitor.hpp>
#include <bg2e/scene/DrawableComponent.hpp>
#include <bg2e/json/JsonNode.hpp>
#include <bg2e/json/JsonParser.hpp>
#include <bg2e/json/NodeReader.hpp>
#include <bg2e/base/Log.hpp>

#include <fstream>
#include <stdexcept>
#include <iterator>

namespace bg2e::db {

static int countJsonNodes(std::shared_ptr<bg2e::json::JsonNode> jsonNode)
{
    json::ObjectReader reader(jsonNode);
    if (!reader.isValid())
    {
        return 0;
    }
    int count = 1;
    if (auto children = reader.getArray("children"))
    {
        for (std::size_t i = 0; i < children->size(); ++i)
        {
            if (auto child = children->getObject(i))
            {
                count += countJsonNodes(child->node());
            }
        }
    }
    return count;
}

static int countEnvironmentImages(std::shared_ptr<bg2e::json::JsonNode> jsonNode)
{
    json::ObjectReader reader(jsonNode);
    if (!reader.isValid())
    {
        return 0;
    }
    int count = 0;

    if (auto components = reader.getArray("components"))
    {
        for (std::size_t i = 0; i < components->size(); ++i)
        {
            auto comp = components->getObject(i);
            if (comp && comp->getString("type").value_or("") == "Environment")
            {
                if (comp->isString("equirectangularTexture"))
                {
                    ++count;
                }
            }
        }
    }

    if (auto children = reader.getArray("children"))
    {
        for (std::size_t i = 0; i < children->size(); ++i)
        {
            if (auto child = children->getObject(i))
            {
                count += countEnvironmentImages(child->node());
            }
        }
    }
    return count;
}

static int countDrawableTextures(std::shared_ptr<bg2e::json::JsonNode> jsonNode, const std::filesystem::path& basePath)
{
    json::ObjectReader reader(jsonNode);
    if (!reader.isValid())
    {
        return 0;
    }
    int count = 0;

    if (auto components = reader.getArray("components"))
    {
        for (std::size_t i = 0; i < components->size(); ++i)
        {
            auto comp = components->getObject(i);
            if (comp && comp->getString("type").value_or("") == "Drawable")
            {
                if (auto name = comp->getString("name"))
                {
                    auto filePath = basePath;
                    filePath.append(*name);
                    filePath.replace_extension(".bg2");
                    count += bg2e::db::countMeshTextures(filePath);
                }
            }
        }
    }

    if (auto children = reader.getArray("children"))
    {
        for (std::size_t i = 0; i < children->size(); ++i)
        {
            if (auto child = children->getObject(i))
            {
                count += countDrawableTextures(child->node(), basePath);
            }
        }
    }
    return count;
}

static int countSceneNodes(bg2e::scene::Node* node)
{
    if (!node)
    {
        return 0;
    }
    int count = 1;
    for (auto& child : node->children())
    {
        count += countSceneNodes(child.get());
    }
    return count;
}

static void validateSceneDocument(const std::shared_ptr<json::JsonNode>& document)
{
    json::ObjectReader reader(document);
    if (!reader.isValid() || !reader.getArray("scene"))
    {
        throw std::invalid_argument("Scene document requires an object containing a scene array");
    }
    if (auto type = reader.getNode("fileType"); type && (!type->isString() || type->stringValue() != "bg2e::scene"))
    {
        throw std::invalid_argument("Unsupported scene document type");
    }
}

std::shared_ptr<json::JsonNode> readSceneDocument(const std::filesystem::path& filePath)
{
    std::ifstream input(filePath, std::ios::binary);
    if (!input)
    {
        throw std::runtime_error("Could not open scene document: " + filePath.string());
    }
    std::string content((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    if (input.bad())
    {
        throw std::runtime_error("Could not read scene document: " + filePath.string());
    }
    auto document = json::JsonParser(content).parse();
    validateSceneDocument(document);
    return document;
}

void writeSceneDocument(std::shared_ptr<json::JsonNode> document, const std::filesystem::path& filePath)
{
    validateSceneDocument(document);
    try
    {
        std::ofstream output;
        output.exceptions(std::ios::failbit | std::ios::badbit);
        output.open(filePath, std::ios::binary);
        output << document->toString();
        output.close();
    }
    catch (const std::ios_base::failure& error)
    {
        throw std::runtime_error("Could not write scene document '" + filePath.string() + "': " + error.what());
    }
}

void saveSceneReferences(scene::Node* root, const std::filesystem::path& filePath,
    scene::SceneProgressCallback onProgress)
{
    if (!root)
    {
        throw std::invalid_argument("Scene reference serialization requires a root");
    }
    scene::SceneSaveProgress progress;
    progress.callback = onProgress;
    progress.total = countSceneNodes(root);
    using namespace json;
    auto document = JSON(JsonObject{
        {"fileType", JSON("bg2e::scene")},
        {"version", JSON(JsonObject{{"major", JSON(1)}, {"minor", JSON(0)}, {"rev", JSON(0)}})},
        {"scene", JSON(JsonList{root->serializeReferences(filePath.parent_path(), onProgress ? &progress : nullptr)})}});
    writeSceneDocument(document, filePath);
}

static int countDrawTextures(std::shared_ptr<json::JsonNode> data, const std::filesystem::path& basePath)
{
    json::ObjectReader reader(data);
    int result = 0;
    if (auto components = reader.getArray("components"))
    {
        for (std::size_t i = 0; i < components->size(); ++i)
        {
            auto component = components->getObject(i);
            if (!component || component->getString("type").value_or("") != "Drawable")
            {
                continue;
            }
            std::filesystem::path path;
            if (auto source = component->getString("path"))
            {
                path = basePath / *source;
            }
            else if (auto name = component->getString("name"))
            {
                path = basePath / *name;
                path.replace_extension(".bg2");
            }
            if (path.extension() == ".bg2")
            {
                result += countMeshTextures(path);
            }
        }
    }
    if (auto children = reader.getArray("children"))
    {
        for (std::size_t i = 0; i < children->size(); ++i)
        {
            if (auto child = children->getObject(i))
            {
                result += countDrawTextures(child->node(), basePath);
            }
        }
    }
    return result;
}

static std::shared_ptr<scene::Scene> loadSceneRepresentation(const std::filesystem::path& filePath,
    draw::Engine* engine, scene::SceneProgressCallback onProgress)
{
    auto document = readSceneDocument(filePath);
    scene::SceneLoadProgress progress;
    progress.callback = onProgress;
    if (onProgress)
    {
        auto nodes = json::ObjectReader(document).getArray("scene");
        for (std::size_t i = 0; i < nodes->size(); ++i)
        {
            auto node = nodes->getObject(i);
            if (node)
            {
                progress.total += countJsonNodes(node->node());
                if (engine)
                {
                    progress.total += countDrawTextures(node->node(), filePath.parent_path());
                }
            }
        }
    }
    auto result = scene::Scene::deserialize(document, filePath.parent_path(), onProgress ? &progress : nullptr);
    if (engine)
    {
        result->rootNode()->loadResources(*engine, onProgress ? &progress : nullptr);
    }
    return result;
}

std::shared_ptr<scene::Scene> loadScene(const std::filesystem::path& filePath, scene::SceneProgressCallback onProgress)
{
    return loadSceneRepresentation(filePath, nullptr, onProgress);
}

std::shared_ptr<scene::Scene> loadScene(const std::filesystem::path& basePath, const std::string& fileName,
    scene::SceneProgressCallback onProgress)
{
    return loadScene(basePath / fileName, onProgress);
}

std::shared_ptr<scene::Scene> loadScene(const std::filesystem::path& filePath, draw::Engine& engine,
    scene::SceneProgressCallback onProgress)
{
    return loadSceneRepresentation(filePath, &engine, onProgress);
}

std::shared_ptr<scene::Scene> loadScene(const std::filesystem::path& basePath, const std::string& fileName,
    draw::Engine& engine, scene::SceneProgressCallback onProgress)
{
    return loadScene(basePath / fileName, engine, onProgress);
}

std::shared_ptr<bg2e::scene::Scene> loadScene(
    const std::filesystem::path& filePath,
    bg2e::render::Engine& engine,
    bg2e::scene::SceneProgressCallback onProgress)
{
    // Preserve bytes so the file size matches the number of characters read.
    std::ifstream inFile(filePath, std::ios::binary);
    if (!inFile.is_open())
    {
        bg2e_log_error << "Could not open scene file at path \"" << filePath << "\"" << bg2e_log_end;
        return nullptr;
    }

    inFile.seekg(0, std::ios::end);
    std::string content;
    content.resize(inFile.tellg());

    inFile.seekg(0, std::ios::beg);
    inFile.read(&content[0], content.size());
    content.resize(static_cast<size_t>(inFile.gcount()));

    auto parser = json::JsonParser(content);
    auto sceneFile = parser.parse();

    if (!sceneFile)
    {
        bg2e_log_error << "Error parsing scene file at path \"" << filePath << "\"" << bg2e_log_end;
        return nullptr;
    }

    // Build progress context and pre-count nodes
    scene::SceneLoadProgress progress;
    if (onProgress)
    {
        progress.callback = onProgress;
        auto basePath = filePath.parent_path();
        json::ObjectReader reader(sceneFile);
        if (auto sceneList = reader.getArray("scene"))
        {
            for (std::size_t i = 0; i < sceneList->size(); ++i)
            {
                auto nodeData = sceneList->getObject(i);
                if (!nodeData)
                {
                    continue;
                }
                progress.total += countJsonNodes(nodeData->node());
                progress.total += countEnvironmentImages(nodeData->node());
                progress.total += countDrawableTextures(nodeData->node(), basePath);
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
    bg2e::scene::SceneProgressCallback onProgress)
{
    return loadScene(basePath / fileName, engine, onProgress);
}

void saveScene(
    bg2e::scene::Node* sceneRoot,
    const std::filesystem::path& filePath,
    bg2e::scene::SceneProgressCallback onProgress)
{
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

    // Enable stream exceptions so callers cannot mistake a failed write for a
    // successful save. Registry-owned shared pointers are released on unwind.
    try
    {
        std::ofstream file;
        file.exceptions(std::ios::failbit | std::ios::badbit);
        file.open(filePath);
        using namespace bg2e::json;
        auto sceneJson = JSON(JsonObject{
            {"fileType", JSON("bg2e::scene")},
            {"version", JSON(JsonObject{
                            {"major", JSON(1)},
                            {"minor", JSON(0)},
                            {"rev", JSON(0)},
                        })},
            {"scene", JSON(JsonList{sceneData})}});
        file << sceneJson->toString();
        file.close();
    }
    catch (const std::ios_base::failure& error)
    {
        throw std::runtime_error("Could not save scene to '" + filePath.string() + "': " + error.what());
    }

    registry.cleanup();
}

void saveScene(
    bg2e::scene::Node* sceneRoot,
    const std::filesystem::path& basePath,
    const std::string& fileName,
    bg2e::scene::SceneProgressCallback onProgress)
{
    saveScene(sceneRoot, basePath / fileName, onProgress);
}

}
