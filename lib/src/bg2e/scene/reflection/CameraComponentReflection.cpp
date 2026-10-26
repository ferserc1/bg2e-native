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
#include <bg2e/scene/CameraComponent.hpp>

#include <memory>
#include <utility>

using namespace bg2e;

namespace {

void replaceProjection(scene::CameraComponent& camera, std::shared_ptr<math::Projection> replacement)
{
    if (const auto* current = camera.projection())
    {
        replacement->setNear(current->near());
        replacement->setFar(current->far());
        replacement->setViewport(current->viewport());
    }
    camera.setProjection(std::move(replacement));
}

reflection::TypeRegistration<scene::CameraComponent> _cameraComponentReflection(
    [](reflection::TypeInfoBuilder<scene::CameraComponent>& t)
    {
        t.displayName("Camera");

        t.polymorphicObject<math::Projection>(
             "projection",
             "bg2e::math::Projection",
             [](const scene::CameraComponent& camera) -> const math::Projection*
             {
                 return camera.projection();
             },
             [](scene::CameraComponent& camera) -> math::Projection*
             {
                 return camera.projection();
             },
             &replaceProjection)
            .displayName("Projection")
            .category("Camera")
            .tooltip("Projection model and its editable parameters")
            .subtype("PerspectiveProjection")
            .subtype("OpticalProjection")
            .subtype("OrthographicProjection");

        t.action("resetProjection", [](scene::CameraComponent* camera)
        {
            replaceProjection(*camera, std::make_shared<math::PerspectiveProjection>());
        })
            .displayName("Reset Projection");
    });

}
