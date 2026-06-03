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

#include <bg2e/scene/FixedScaleTransformController.hpp>
#include <bg2e/scene/ComponentFactoryRegistry.hpp>
#include <bg2e/json/JsonNode.hpp>
#include <bg2e/scene/Node.hpp>
#include <bg2e/scene/TransformComponent.hpp>
#include <bg2e/scene/Scene.hpp>

namespace bg2e::scene {

void FixedScaleTransformControllerComponent::deserialize(std::shared_ptr<json::JsonNode> jsonData, const std::filesystem::path&)
{
    if (!jsonData || !jsonData->isObject())
        return;

    auto& obj = jsonData->objectValue();
    if (obj.count("scale"))
    {
        _scale = obj["scale"]->numberValue(_scale);
    }
}

std::shared_ptr<json::JsonNode> FixedScaleTransformControllerComponent::serialize(const std::filesystem::path& basePath)
{
    using namespace bg2e::json;
    auto compData = Component::serialize(basePath);
    compData->objectValue()["scale"] = JSON(_scale);
    return compData;
}

void FixedScaleTransformControllerComponent::update([[maybe_unused]] float delta)
{
    auto node = ownerNode();
    if (!node)
    {
        return;
    }

    auto trx = node->transform();
    if (!trx)
    {
        return;
    }

    auto scene = node->sceneRoot()->scene();
    if (!scene)
    {
        return;
    }

    auto mainCamera = scene->mainCamera();
    if (!mainCamera)
    {
        return;
    }

    glm::mat4 currentMatrix = trx->matrix();
    glm::vec3 worldPosition = ownerNode()->worldPosition();
    glm::mat4 viewMatrix = mainCamera->ownerNode()->invertedWorldMatrix();
    glm::mat4 projectionMatrix = mainCamera->projectionMatrix();
    glm::vec4 viewPos = viewMatrix * glm::vec4(worldPosition, 1.0f);
    float viewDepth = std::abs(viewPos.z);
    float scale = _scale * viewDepth / projectionMatrix[1][1];

    glm::mat4 finalMatrix = currentMatrix;
    finalMatrix[0] = glm::normalize(glm::vec4(glm::vec3(currentMatrix[0]), 0.0f)) * scale;
    finalMatrix[1] = glm::normalize(glm::vec4(glm::vec3(currentMatrix[1]), 0.0f)) * scale;
    finalMatrix[2] = glm::normalize(glm::vec4(glm::vec3(currentMatrix[2]), 0.0f)) * scale;
    finalMatrix[3] = currentMatrix[3];

    trx->setMatrix(finalMatrix);
}

BG2E_SCENE_REGISTER_COMPONENT(FixedScaleTransformControllerComponent);

}
