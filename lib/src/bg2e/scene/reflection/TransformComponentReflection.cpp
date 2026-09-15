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

#include <bg2e/reflection/Registration.hpp>
#include <bg2e/scene/TransformComponent.hpp>

using namespace bg2e;

namespace {

reflection::TypeRegistration<scene::TransformComponent> _transformReflection(
    [](reflection::TypeInfoBuilder<scene::TransformComponent>& t) {
        t.displayName("Transform");

        t.property("translation", &scene::TransformComponent::translation,
                                  static_cast<scene::TransformComponent*(scene::TransformComponent::*)(const glm::vec3&)>(
                                      &scene::TransformComponent::setTranslation))
            .displayName("Translation")
            .category("Transform");

        t.property("rotation", &scene::TransformComponent::eulerRotation,
                               &scene::TransformComponent::setEulerRotation)
            .displayName("Rotation")
            .category("Transform")
            .tooltip("Euler rotation in degrees");

        t.property("scale", &scene::TransformComponent::scaleValue,
                            static_cast<scene::TransformComponent*(scene::TransformComponent::*)(const glm::vec3&)>(
                                &scene::TransformComponent::setScale))
            .displayName("Scale")
            .category("Transform");

        t.action("setIdentity", &scene::TransformComponent::setIdentity)
            .displayName("Set Identity")
            .category("Transform")
            .tooltip("Reset the transform to the identity matrix");
    });

}
