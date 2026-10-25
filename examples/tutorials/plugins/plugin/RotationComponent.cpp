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

#include "RotationComponent.hpp"
#include "PluginComponents.hpp"

#include <bg2e/reflection/Builder.hpp>

namespace tutorial {

void registerRotation(bg2e::plugin::ComponentRegistrar& registrar)
{
    bg2e::reflection::TypeInfoBuilder<RotationComponent> type(RotationComponent::staticTypeName());
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
    registrar.registerComponent<RotationComponent>(type.build());
}

}
