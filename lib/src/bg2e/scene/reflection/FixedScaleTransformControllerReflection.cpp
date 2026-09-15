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
#include <bg2e/scene/FixedScaleTransformController.hpp>

using namespace bg2e;

namespace {

reflection::TypeRegistration<scene::FixedScaleTransformControllerComponent> _fixedScaleReflection(
    [](reflection::TypeInfoBuilder<scene::FixedScaleTransformControllerComponent>& t) {
        t.displayName("Fixed Scale Transform");
        t.property("scale", &scene::FixedScaleTransformControllerComponent::scale,
                            &scene::FixedScaleTransformControllerComponent::setScale)
            .displayName("Scale")
            .category("Transform")
            .range(0.0, 1000.0)
            .step(0.01)
            .drag();
    });

}
