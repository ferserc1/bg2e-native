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
#include <bg2e/scene/PolarTransformController.hpp>

using namespace bg2e;

namespace {

reflection::TypeRegistration<scene::PolarTransformControllerComponent> _polarTransformReflection(
    [](reflection::TypeInfoBuilder<scene::PolarTransformControllerComponent>& t) {
        t.displayName("Polar Transform Controller");

        t.property("enabled", &scene::PolarTransformControllerComponent::enabled,
                              &scene::PolarTransformControllerComponent::setEnabled)
            .displayName("Enabled").category("General").checkbox();
        t.property("azimuth", &scene::PolarTransformControllerComponent::azimuth,
                              &scene::PolarTransformControllerComponent::setAzimuth)
            .displayName("Azimuth").category("Polar Position").range(0.0, 360.0).angle();
        t.property("elevation", &scene::PolarTransformControllerComponent::elevation,
                                &scene::PolarTransformControllerComponent::setElevation)
            .displayName("Elevation").category("Polar Position").range(-90.0, 90.0).angle();
        t.property("distance", &scene::PolarTransformControllerComponent::distance,
                               &scene::PolarTransformControllerComponent::setDistance)
            .displayName("Distance").category("Polar Position").range(0.0, 100000.0).step(0.1).drag();
        t.property("target", &scene::PolarTransformControllerComponent::target,
                             &scene::PolarTransformControllerComponent::setTarget)
            .displayName("Target").category("Polar Position");

        t.property("eulerX", &scene::PolarTransformControllerComponent::eulerX,
                             &scene::PolarTransformControllerComponent::setEulerX)
            .displayName("X").category("Euler Rotation").range(-180.0, 180.0).angle();
        t.property("eulerY", &scene::PolarTransformControllerComponent::eulerY,
                             &scene::PolarTransformControllerComponent::setEulerY)
            .displayName("Y").category("Euler Rotation").range(-180.0, 180.0).angle();
        t.property("eulerZ", &scene::PolarTransformControllerComponent::eulerZ,
                             &scene::PolarTransformControllerComponent::setEulerZ)
            .displayName("Z").category("Euler Rotation").range(-180.0, 180.0).angle();
    });

}
