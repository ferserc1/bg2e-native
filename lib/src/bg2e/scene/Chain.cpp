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

#include <bg2e/scene/Chain.hpp>
#include <bg2e/scene/ChainJoint.hpp>
#include <bg2e/scene/ComponentFactoryRegistry.hpp>
#include <bg2e/scene/Node.hpp>
#include <bg2e/scene/TransformComponent.hpp>

#include <cstddef>

namespace bg2e::scene {

std::shared_ptr<Component> ChainComponent::clone() const
{
    auto copy = std::make_shared<ChainComponent>(*this);
    copy->_owner = nullptr;
    return copy;
}

void ChainComponent::animate([[maybe_unused]] float delta)
{
    auto node = ownerNode();
    if (!node)
    {
        return;
    }

    glm::mat4 matrix(1.0f);
    const auto& children = node->children();
    for (std::size_t index = 0; index < children.size(); ++index)
    {
        auto* child = children[index].get();
        auto* inputJoint = child->getComponent<InputChainJointComponent>();
        auto* outputJoint = child->getComponent<OutputChainJointComponent>();

        if (index > 0 && inputJoint)
        {
            inputJoint->joint().applyTransform(matrix);
        }
        else
        {
            matrix = glm::mat4(1.0f);
        }

        if (auto* transform = child->transform())
        {
            transform->setMatrix(matrix);
        }

        if (outputJoint)
        {
            outputJoint->joint().applyTransform(matrix);
        }
    }
}

void ChainComponent::deserialize(
    std::shared_ptr<json::JsonNode> jsonData,
    const std::filesystem::path& basePath,
    render::Engine& engine)
{
    Component::deserialize(jsonData, basePath, engine);
}

std::shared_ptr<json::JsonNode> ChainComponent::serialize(const std::filesystem::path& basePath)
{
    return Component::serialize(basePath);
}

BG2E_SCENE_REGISTER_COMPONENT(ChainComponent);

}
