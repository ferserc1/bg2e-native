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

#include <bg2e/scene/EnvironmentComponent.hpp>
#include <bg2e/scene/ComponentFactoryRegistry.hpp>
#include <bg2e/scene/Node.hpp>
#include <bg2e/scene/Scene.hpp>
#include <bg2e/utils/TextureCache.hpp>

namespace bg2e::scene {

EnvironmentComponent::EnvironmentComponent()
{

}

EnvironmentComponent::EnvironmentComponent(const std::string& img)
    :_environmentImage{img}
{
    _imgHash = std::hash<std::string>{}(img);
}

EnvironmentComponent::EnvironmentComponent(const std::filesystem::path& resourcePath, const std::string& file)
{
    auto fullPath = resourcePath;
    fullPath.append(file);
    _environmentImage = fullPath.string();
    _imgHash = std::hash<std::string>{}(_environmentImage);
}

std::shared_ptr<Component> EnvironmentComponent::clone() const
{
    auto copy = std::make_shared<EnvironmentComponent>(*this);
    copy->_owner = nullptr;
    return copy;
}

EnvironmentComponent::~EnvironmentComponent()
{

}

void EnvironmentComponent::deserialize(std::shared_ptr<json::JsonNode> jsonData, const std::filesystem::path& basePath, [[maybe_unused]] render::Engine& engine)
{
    if (!jsonData || !jsonData->isObject())
    {
        return;
    }

    auto& obj = jsonData->objectValue();

    if (obj.count("equirectangularTexture"))
    {
        auto textureName = obj["equirectangularTexture"]->stringValue();
        setEnvironmentImage(basePath, textureName);
    }
}

void EnvironmentComponent::deserializeWithProgress(
    std::shared_ptr<json::JsonNode> jsonData,
    const std::filesystem::path& basePath,
    render::Engine& engine,
    SceneLoadProgress* progress
) {
    deserialize(jsonData, basePath, engine);

    if (!_environmentImage.empty())
    {
        utils::TextureCache::get().load(&engine, _environmentImage);
        if (progress && progress->callback)
        {
            progress->callback(_environmentImage, progress->loaded, progress->total);
        }
        if (progress) { ++progress->loaded; }
    }
}

std::shared_ptr<json::JsonNode> EnvironmentComponent::serialize(const std::filesystem::path& basePath)
{
    using namespace bg2e::json;
    auto compData = Component::serialize(basePath);
    JsonObject & obj = compData->objectValue();
    std::filesystem::path dstFilePath = basePath;
    std::filesystem::path srcFilePath = _environmentImage;
    auto fileName = srcFilePath.filename();
    dstFilePath += fileName;
    if (srcFilePath != dstFilePath)
    {
        std::filesystem::copy(srcFilePath, dstFilePath, std::filesystem::copy_options::overwrite_existing);
    }
    obj["equirectangularTexture"] = JSON(fileName.string());
    
    // The following properties are deprecated and are not serialized:
    // - irradianceIntensity
    // - showSkybox
    // - cubemapSize
    // - irradianceMapSize
    // - specularMapSize
    // - specularMapL2Size

    return compData;
}

BG2E_SCENE_REGISTER_COMPONENT(EnvironmentComponent);

}
