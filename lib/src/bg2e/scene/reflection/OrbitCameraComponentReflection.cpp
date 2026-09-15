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
#include <bg2e/scene/OrbitCameraComponent.hpp>

using namespace bg2e;

namespace {

reflection::TypeRegistration<scene::OrbitCameraComponent> _orbitCameraReflection(
    [](reflection::TypeInfoBuilder<scene::OrbitCameraComponent>& t) {
        t.displayName("Orbit Camera Controller");

        t.property("enabled", &scene::OrbitCameraComponent::enabled, &scene::OrbitCameraComponent::setEnabled)
            .displayName("Enabled").category("General").checkbox();

        t.property("rotation", &scene::OrbitCameraComponent::rotation, &scene::OrbitCameraComponent::setRotation)
            .displayName("Rotation").category("View").tooltip("Pitch and yaw in degrees");
        t.property("distance", &scene::OrbitCameraComponent::distance, &scene::OrbitCameraComponent::setDistance)
            .displayName("Distance").category("View").range(0.0, 100000.0).step(0.1).drag();
        t.property("center", &scene::OrbitCameraComponent::center, &scene::OrbitCameraComponent::setCenter)
            .displayName("Center").category("View");

        t.property("rotationSpeed", &scene::OrbitCameraComponent::rotationSpeed, &scene::OrbitCameraComponent::setRotationSpeed)
            .displayName("Rotation Speed").category("Speeds").range(0.0, 100.0).step(0.01).drag();
        t.property("wheelSpeed", &scene::OrbitCameraComponent::wheelSpeed, &scene::OrbitCameraComponent::setWheelSpeed)
            .displayName("Wheel Speed").category("Speeds").range(0.0, 100.0).step(0.01).drag();
        t.property("panSpeed", &scene::OrbitCameraComponent::panSpeed, &scene::OrbitCameraComponent::setPanSpeed)
            .displayName("Pan Speed").category("Speeds").range(0.0, 100.0).step(0.01).drag();
        t.property("displacementSpeed", &scene::OrbitCameraComponent::displacementSpeed, &scene::OrbitCameraComponent::setDisplacementSpeed)
            .displayName("Displacement Speed").category("Speeds").range(0.0, 100000.0).step(0.01).drag();

        t.property("minFocus", &scene::OrbitCameraComponent::minFocus, &scene::OrbitCameraComponent::setMinFocus)
            .displayName("Minimum Focus").category("Limits").range(0.0, 100000.0).step(0.1).drag();
        t.property("minPitch", &scene::OrbitCameraComponent::minPitch, &scene::OrbitCameraComponent::setMinPitch)
            .displayName("Minimum Pitch").category("Limits").range(-90.0, 90.0).angle();
        t.property("maxPitch", &scene::OrbitCameraComponent::maxPitch, &scene::OrbitCameraComponent::setMaxPitch)
            .displayName("Maximum Pitch").category("Limits").range(-90.0, 90.0).angle();
        t.property("minDistance", &scene::OrbitCameraComponent::minDistance, &scene::OrbitCameraComponent::setMinDistance)
            .displayName("Minimum Distance").category("Limits").range(0.0, 100000.0).step(0.1).drag();
        t.property("maxDistance", &scene::OrbitCameraComponent::maxDistance, &scene::OrbitCameraComponent::setMaxDistance)
            .displayName("Maximum Distance").category("Limits").range(0.0, 100000.0).step(0.1).drag();

        t.property("minX", &scene::OrbitCameraComponent::minX, &scene::OrbitCameraComponent::setMinX)
            .displayName("Minimum X").category("Bounds").step(0.1).drag();
        t.property("maxX", &scene::OrbitCameraComponent::maxX, &scene::OrbitCameraComponent::setMaxX)
            .displayName("Maximum X").category("Bounds").step(0.1).drag();
        t.property("minY", &scene::OrbitCameraComponent::minY, &scene::OrbitCameraComponent::setMinY)
            .displayName("Minimum Y").category("Bounds").step(0.1).drag();
        t.property("maxY", &scene::OrbitCameraComponent::maxY, &scene::OrbitCameraComponent::setMaxY)
            .displayName("Maximum Y").category("Bounds").step(0.1).drag();
        t.property("minZ", &scene::OrbitCameraComponent::minZ, &scene::OrbitCameraComponent::setMinZ)
            .displayName("Minimum Z").category("Bounds").step(0.1).drag();
        t.property("maxZ", &scene::OrbitCameraComponent::maxZ, &scene::OrbitCameraComponent::setMaxZ)
            .displayName("Maximum Z").category("Bounds").step(0.1).drag();

        t.property("initialRotation", &scene::OrbitCameraComponent::initialRotation, &scene::OrbitCameraComponent::setInitialRotation)
            .displayName("Rotation").category("Initial Values").tooltip("Initial pitch and yaw in degrees");
        t.property("initialDistance", &scene::OrbitCameraComponent::initialDistance, &scene::OrbitCameraComponent::setInitialDistance)
            .displayName("Distance").category("Initial Values").range(0.0, 100000.0).step(0.1).drag();
        t.property("initialCenter", &scene::OrbitCameraComponent::initialCenter, &scene::OrbitCameraComponent::setInitialCenter)
            .displayName("Center").category("Initial Values");

        t.action("reset", &scene::OrbitCameraComponent::reset)
            .displayName("Reset").category("General")
            .tooltip("Restore the initial rotation, distance, and center");
    });

}
