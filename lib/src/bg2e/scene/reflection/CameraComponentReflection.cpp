/*
 *    business grade graphic engine (bg2 engine)
 *    Copyright (C) 2026  Fernando Serrano Carpena
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 */

#include <bg2e/reflection/Registration.hpp>
#include <bg2e/scene/CameraComponent.hpp>

#include <memory>
#include <utility>

using namespace bg2e;

namespace {

reflection::TypeRegistration<scene::CameraComponent> _cameraComponentReflection(
    [](reflection::TypeInfoBuilder<scene::CameraComponent>& t) {
        t.displayName("Camera");

        t.polymorphicObject<math::Projection>(
            "projection",
            "bg2e::math::Projection",
            [](const scene::CameraComponent& camera) -> const math::Projection * {
                return camera.projection();
            },
            [](scene::CameraComponent& camera) -> math::Projection * {
                return camera.projection();
            },
            [](scene::CameraComponent& camera, std::shared_ptr<math::Projection> replacement) {
                if (const auto * current = camera.projection())
                {
                    replacement->setNear(current->near());
                    replacement->setFar(current->far());
                    replacement->setViewport(current->viewport());
                }
                camera.setProjection(std::move(replacement));
            }
        )
            .displayName("Projection")
            .category("Camera")
            .tooltip("Projection model and its editable parameters")
            .subtype("PerspectiveProjection")
            .subtype("OpticalProjection");
    });

}
