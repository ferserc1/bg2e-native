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
#include <utility>
#include <bg2e/json/NodeReader.hpp>
#include <bg2e/scene/ComponentFactoryRegistry.hpp>
#include <bg2e/scene/Node.hpp>
#include <bg2e/scene/Scene.hpp>
#include <bg2e/utils/TextureCache.hpp>

#include <cmath>
#include <algorithm>
#include <cctype>
#include <stdexcept>

namespace bg2e::scene {

void EnvironmentComponent::exchangeValues(EnvironmentComponent& other) noexcept
{
    using std::swap;
    swap(_environmentImage, other._environmentImage);
    swap(_imgHash, other._imgHash);
    swap(_skyboxBlur, other._skyboxBlur);
    swap(_skyboxVisible, other._skyboxVisible);
    swap(_skyHorizonColor, other._skyHorizonColor);
    swap(_skyZenithColor, other._skyZenithColor);
    swap(_groundColor, other._groundColor);
}


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

void EnvironmentComponent::setSkyHorizonColor(const base::Color& color)
{
    if (!std::isfinite(color.r) || !std::isfinite(color.g) || !std::isfinite(color.b) ||
        color.r < 0 || color.r > 1 || color.g < 0 || color.g > 1 || color.b < 0 || color.b > 1)
    {
        throw std::invalid_argument("Environment colors must have finite linear RGB values in [0, 1]");
    }
    _skyHorizonColor = base::Color(color.r, color.g, color.b);
}

void EnvironmentComponent::setSkyZenithColor(const base::Color& color)
{
    if (!std::isfinite(color.r) || !std::isfinite(color.g) || !std::isfinite(color.b) ||
        color.r < 0 || color.r > 1 || color.g < 0 || color.g > 1 || color.b < 0 || color.b > 1)
    {
        throw std::invalid_argument("Environment colors must have finite linear RGB values in [0, 1]");
    }
    _skyZenithColor = base::Color(color.r, color.g, color.b);
}

void EnvironmentComponent::setGroundColor(const base::Color& color)
{
    if (!std::isfinite(color.r) || !std::isfinite(color.g) || !std::isfinite(color.b) ||
        color.r < 0 || color.r > 1 || color.g < 0 || color.g > 1 || color.b < 0 || color.b > 1)
    {
        throw std::invalid_argument("Environment colors must have finite linear RGB values in [0, 1]");
    }
    _groundColor = base::Color(color.r, color.g, color.b);
}

void EnvironmentComponent::setSkyboxBlur(float blur)
{
    if (!std::isfinite(blur) || blur < 0 || blur > 1)
    {
        throw std::invalid_argument("Skybox blur must be finite and normalized to [0, 1]");
    }
    _skyboxBlur = blur;
}

void EnvironmentComponent::deserialize(std::shared_ptr<json::JsonNode> jsonData, const std::filesystem::path& basePath)
{
    deserializeCommon(jsonData);
    json::ObjectReader reader(jsonData);
    setSkyHorizonColor(reader.getColor("skyHorizonColor").value_or(base::Color{0.55f, 0.65f, 0.8f}));
    setSkyZenithColor(reader.getColor("skyZenithColor").value_or(base::Color{0.06f, 0.22f, 0.6f}));
    setGroundColor(reader.getColor("groundColor").value_or(base::Color{0.04f, 0.035f, 0.025f}));
    setSkyboxBlur(float(reader.getNumber("skyboxBlur").value_or(0.0)));
    setSkyboxVisible(reader.getBool("skyboxVisible").value_or(true));
    auto textureName = reader.getString("equirectangularTexture");
    if (!textureName)
    {
        textureName = reader.getString("environmentImage");
    }
    setEnvironmentImage(!textureName || textureName->empty() ? std::string{} :
        std::filesystem::absolute(basePath / *textureName).lexically_normal().string());
}

std::shared_ptr<json::JsonNode> EnvironmentComponent::serialize(const std::filesystem::path& basePath)
{
    using namespace bg2e::json;
    auto compData = Component::serialize(basePath);
    JsonObject& obj = compData->objectValue();
    obj["skyHorizonColor"] = JSON(_skyHorizonColor);
    obj["skyZenithColor"] = JSON(_skyZenithColor);
    obj["groundColor"] = JSON(_groundColor);
    obj["skyboxBlur"] = JSON(_skyboxBlur);
    obj["skyboxVisible"] = JSON(_skyboxVisible);
    std::filesystem::path dstFilePath = basePath;
    std::filesystem::path srcFilePath = _environmentImage;
    auto fileName = srcFilePath.filename();
    dstFilePath += fileName;
    if (!srcFilePath.empty() && srcFilePath != dstFilePath)
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
    result->objectValue()["skyHorizonColor"] = json::JSON(_skyHorizonColor);
    result->objectValue()["skyZenithColor"] = json::JSON(_skyZenithColor);
    result->objectValue()["groundColor"] = json::JSON(_groundColor);
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
