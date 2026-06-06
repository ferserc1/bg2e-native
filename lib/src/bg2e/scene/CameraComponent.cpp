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

#include <bg2e/scene/CameraComponent.hpp>
#include <bg2e/scene/ComponentFactoryRegistry.hpp>

#include "bg2e/scene/Node.hpp"

namespace bg2e::scene {

CameraComponent::CameraComponent()
{

}

CameraComponent::~CameraComponent()
{

}

void CameraComponent::resizeViewport(const math::Viewport& vp)
{
    auto proj = _camera.projection();
    if (proj)
    {
        proj->setViewport(vp);
    }
}

const glm::mat4& CameraComponent::viewMatrix() const
{
    if (ownerNode())
    {
        _viewMatrix = ownerNode()->invertedWorldMatrix();
    }
    return _viewMatrix;
}

void CameraComponent::update(float /* delta */)
{
    _camera.updateProjectionMatrix();
}

void CameraComponent::deserialize(std::shared_ptr<json::JsonNode> jsonData, const std::filesystem::path& /* basePath */, [[maybe_unused]] render::Engine& engine)
{
    if (!jsonData || !jsonData->isObject())
        return;

    auto& obj = jsonData->objectValue();
    if (obj.count("cameraData"))
    {
        _camera.deserialize(obj["cameraData"]);
    }
}

std::shared_ptr<json::JsonNode> CameraComponent::serialize(const std::filesystem::path& basePath)
{
    using namespace bg2e::json;
    auto compData = Component::serialize(basePath);
    JsonObject & obj = compData->objectValue();
    
    obj["cameraData"] = _camera.serialize();

    return compData;
}

BG2E_SCENE_REGISTER_COMPONENT(CameraComponent);

}
