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

#include <bg2e/scene/Node.hpp>
#include <bg2e/scene/ComponentFactoryRegistry.hpp>
#include <bg2e/reflection/Registration.hpp>

#include <algorithm>
#include <cmath>
#include <memory>

namespace tutorial {

class RotationComponent final : public bg2e::scene::Component {
  public:
    BG2E_COMPONENT_TYPE_NAME("tutorial.Rotation");

    enum class Axis
    {
        X,
        Y,
        Z
    };

    RotationComponent()
    {
        // This example edits live scenes; it does not implement scene-file IO.
        setIgnoreSerialization(true);
    }

    Axis axis() const
    {
        return _axis;
    }
    void setAxis(Axis value)
    {
        if (value == Axis::X || value == Axis::Y || value == Axis::Z)
        {
            _axis = value;
        }
    }
    float speed() const
    {
        return _speed;
    }
    void setSpeed(float value)
    {
        if (std::isfinite(value))
        {
            _speed = std::clamp(value, -360.0f, 360.0f);
        }
    }

    void toggleRotation()
    {
        _rotating = !_rotating;
    }

    void animate(float deltaSeconds) override
    {
        auto* node = ownerNode();
        auto* transform = node ? node->transform() : nullptr;
        if (!_rotating || !transform || !std::isfinite(deltaSeconds) || deltaSeconds <= 0.0f)
        {
            return;
        }
        glm::vec3 direction(0.0f);
        switch (_axis)
        {
        case Axis::X:
            direction.x = 1.0f;
            break;
        case Axis::Y:
            direction.y = 1.0f;
            break;
        case Axis::Z:
            direction.z = 1.0f;
            break;
        }
        transform->rotate(glm::radians(_speed * deltaSeconds), direction);
    }

    std::shared_ptr<bg2e::scene::Component> clone() const override
    {
        auto copy = std::make_shared<RotationComponent>();
        copy->setAxis(_axis);
        copy->setSpeed(_speed);
        copy->_rotating = _rotating;
        copy->setPriority(priority());
        copy->setIgnoreSerialization(ignoreSerialization());
        return copy;
    }

  private:
    Axis _axis = Axis::Y;
    float _speed = 45.0f;
    bool _rotating = true;
};

inline void registerRotationComponent()
{
    // Function-local statics register once, explicitly before application startup.
    static bg2e::reflection::TypeRegistration<RotationComponent> reflectionRegistration(
        [](bg2e::reflection::TypeInfoBuilder<RotationComponent>& type)
        {
            type.displayName("Rotation (tutorial)");
            type.property("axis", &RotationComponent::axis, &RotationComponent::setAxis)
                .displayName("Axis").category("Rotation").combo()
                .enumValue("X", RotationComponent::Axis::X)
                .enumValue("Y", RotationComponent::Axis::Y)
                .enumValue("Z", RotationComponent::Axis::Z)
                .tooltip("Rotate around this local axis");
            type.property("speed", &RotationComponent::speed, &RotationComponent::setSpeed)
                .displayName("Speed (degrees/second)").category("Rotation")
                .drag().range(-360.0, 360.0).step(1.0)
                .tooltip("Negative values reverse rotation; zero pauses it");
            type.action("toggleRotation", &RotationComponent::toggleRotation)
                .displayName("Toggle rotation").category("Rotation")
                .tooltip("Pause or resume rotation without changing the speed or transform");
        });
    static bg2e::scene::RegisterComponent<RotationComponent> factoryRegistration;
    (void)reflectionRegistration;
    (void)factoryRegistration;
}

}
