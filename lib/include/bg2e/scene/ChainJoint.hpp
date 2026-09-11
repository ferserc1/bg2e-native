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

#pragma once

#include <bg2e/base/Joint.hpp>
#include <bg2e/scene/Component.hpp>

namespace bg2e::scene {

class BG2E_API ChainJointComponent : public Component {
public:
    base::LinkJoint& joint() { return _joint; }
    const base::LinkJoint& joint() const { return _joint; }
    void setJoint(const base::LinkJoint& value) { _joint = value; }

    void deserialize(std::shared_ptr<json::JsonNode> jsonData, const std::filesystem::path& basePath, render::Engine& engine) override;
    std::shared_ptr<json::JsonNode> serialize(const std::filesystem::path& basePath) override;

protected:
    base::LinkJoint _joint;
};

class BG2E_API InputChainJointComponent final : public ChainJointComponent {
public:
    BG2E_COMPONENT_TYPE_NAME("InputChainJoint");

    InputChainJointComponent();

    std::shared_ptr<Component> clone() const override;
};

class BG2E_API OutputChainJointComponent final : public ChainJointComponent {
public:
    BG2E_COMPONENT_TYPE_NAME("OutputChainJoint");

    OutputChainJointComponent();

    std::shared_ptr<Component> clone() const override;
};

}
