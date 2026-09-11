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

#include <bg2e/scene/ChainJoint.hpp>
#include <bg2e/scene/ComponentFactoryRegistry.hpp>

namespace bg2e::scene {

void ChainJointComponent::deserialize(
    std::shared_ptr<json::JsonNode> jsonData,
    const std::filesystem::path& basePath,
    render::Engine& engine)
{
    Component::deserialize(jsonData, basePath, engine);

    if (!jsonData || !jsonData->isObject())
    {
        return;
    }

    auto& object = jsonData->objectValue();
    if (!object.count("joint"))
    {
        return;
    }

    auto joint = base::Joint::factory(object["joint"]);
    auto linkJoint = std::dynamic_pointer_cast<base::LinkJoint>(joint);
    if (linkJoint)
    {
        _joint = *linkJoint;
    }
}

std::shared_ptr<json::JsonNode> ChainJointComponent::serialize(const std::filesystem::path& basePath)
{
    auto componentData = Component::serialize(basePath);
    componentData->objectValue()["joint"] = _joint.serialize();
    return componentData;
}

InputChainJointComponent::InputChainJointComponent()
{
    _joint.setTransformOrder(base::LinkTransformOrder::RotateTranslate);
}

std::shared_ptr<Component> InputChainJointComponent::clone() const
{
    auto copy = std::make_shared<InputChainJointComponent>(*this);
    copy->_owner = nullptr;
    return copy;
}

OutputChainJointComponent::OutputChainJointComponent()
{
    _joint.setTransformOrder(base::LinkTransformOrder::TranslateRotate);
}

std::shared_ptr<Component> OutputChainJointComponent::clone() const
{
    auto copy = std::make_shared<OutputChainJointComponent>(*this);
    copy->_owner = nullptr;
    return copy;
}

BG2E_SCENE_REGISTER_COMPONENT(InputChainJointComponent);
BG2E_SCENE_REGISTER_COMPONENT(OutputChainJointComponent);

}
