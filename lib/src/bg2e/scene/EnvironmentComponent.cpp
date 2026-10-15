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
#include <bg2e/json/NodeReader.hpp>
#include <bg2e/scene/ComponentFactoryRegistry.hpp>
#include <bg2e/scene/Node.hpp>
#include <bg2e/scene/Scene.hpp>
#include <bg2e/utils/TextureCache.hpp>

#include <cmath>
#include <stdexcept>

namespace bg2e::scene {

EnvironmentComponent::EnvironmentComponent()
{
}

EnvironmentComponent::EnvironmentComponent(const std::string& img)
    : _environmentImage{img}
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

void EnvironmentComponent::setEnvironmentImage(const std::string& img)
{
    _environmentImage = img;
    _imgHash = std::hash<std::string>{}(img);
}

void EnvironmentComponent::setSkyboxBlur(float blur)
{
    if (!std::isfinite(blur) || blur < 0 || blur > 1)
    {
        throw std::invalid_argument("Skybox blur must be finite and normalized to [0, 1]");
    }
    _skyboxBlur = blur;
}

void EnvironmentComponent::deserialize(std::shared_ptr<json::JsonNode> jsonData, const std::filesystem::path& basePath, render::Engine&)
{
    deserialize(jsonData, basePath);
}

void EnvironmentComponent::deserialize(std::shared_ptr<json::JsonNode> jsonData, const std::filesystem::path& basePath)
{
    deserializeCommon(jsonData);
    json::ObjectReader reader(jsonData);
    setSkyboxBlur(float(reader.getNumber("skyboxBlur").value_or(0.0)));
    setSkyboxVisible(reader.getBool("skyboxVisible").value_or(true));
    if (auto textureName = reader.getString("equirectangularTexture"))
    {
        setEnvironmentImage(textureName->empty() ? std::string{} :
            std::filesystem::absolute(basePath / *textureName).lexically_normal().string());
    }
}

void EnvironmentComponent::deserializeWithProgress(
    std::shared_ptr<json::JsonNode> jsonData,
    const std::filesystem::path& basePath,
    render::Engine& engine,
    SceneLoadProgress* progress)
{
    deserialize(jsonData, basePath, engine);

    if (!_environmentImage.empty())
    {
        utils::TextureCache::get().load(&engine, _environmentImage);
        if (progress && progress->callback)
        {
            progress->callback(_environmentImage, progress->loaded, progress->total);
        }
        if (progress)
        {
            ++progress->loaded;
        }
    }
}

std::shared_ptr<json::JsonNode> EnvironmentComponent::serialize(const std::filesystem::path& basePath)
{
    using namespace bg2e::json;
    auto compData = Component::serialize(basePath);
    JsonObject& obj = compData->objectValue();
    obj["skyboxBlur"] = JSON(_skyboxBlur);
    obj["skyboxVisible"] = JSON(_skyboxVisible);
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

std::shared_ptr<json::JsonNode> EnvironmentComponent::serializeReferences(const std::filesystem::path& basePath)
{
    auto result = Component::serialize(basePath);
    result->objectValue()["skyboxBlur"] = json::JSON(_skyboxBlur);
    result->objectValue()["skyboxVisible"] = json::JSON(_skyboxVisible);
    if (!_environmentImage.empty())
    {
        const auto source = std::filesystem::absolute(_environmentImage).lexically_normal();
        const auto base = basePath.empty() ? std::filesystem::current_path() : std::filesystem::absolute(basePath);
        const auto relative = source.lexically_relative(base.lexically_normal());
        result->objectValue()["equirectangularTexture"] = json::JSON((relative.empty() ? source : relative).generic_string());
    }
    return result;
}

BG2E_SCENE_REGISTER_COMPONENT(EnvironmentComponent);

}
