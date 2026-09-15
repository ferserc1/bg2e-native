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
#include <bg2e/math/projections.hpp>

using namespace bg2e;

namespace {

constexpr const char * projectionType = "bg2e::math::Projection";
constexpr const char * perspectiveType = "bg2e::math::PerspectiveProjection";
constexpr const char * opticalType = "bg2e::math::OpticalProjection";

reflection::TypeRegistration<math::Projection> _projectionReflection(
    projectionType,
    [](reflection::TypeInfoBuilder<math::Projection>& t) {
        t.displayName("Projection");

        t.property("near", &math::Projection::near, &math::Projection::setNear)
            .displayName("Near")
            .category("Clipping")
            .range(0.001, 100000.0)
            .step(0.01)
            .drag();

        t.property("far", &math::Projection::far, &math::Projection::setFar)
            .displayName("Far")
            .category("Clipping")
            .range(0.001, 1000000.0)
            .step(1.0)
            .drag();
    });

reflection::TypeRegistration<math::PerspectiveProjection> _perspectiveReflection(
    perspectiveType,
    [](reflection::TypeInfoBuilder<math::PerspectiveProjection>& t) {
        t.displayName("Perspective Projection");
        t.property("fov", &math::PerspectiveProjection::fov,
                          &math::PerspectiveProjection::setFov)
            .displayName("Field of View")
            .category("Perspective")
            .range(1.0, 179.0)
            .angle();
    });

reflection::TypeRegistration<math::OpticalProjection> _opticalReflection(
    opticalType,
    [](reflection::TypeInfoBuilder<math::OpticalProjection>& t) {
        t.displayName("Optical Projection");
        t.property("focalLength", &math::OpticalProjection::focalLength,
                                  &math::OpticalProjection::setFocalLength)
            .displayName("Focal Length")
            .category("Optics")
            .range(0.1, 1000.0)
            .step(0.1)
            .drag();

        t.property("frameSize", &math::OpticalProjection::frameSize,
                               &math::OpticalProjection::setFrameSize)
            .displayName("Frame Size")
            .category("Optics")
            .range(0.1, 1000.0)
            .step(0.1)
            .drag();
    });

struct ProjectionSubtypeRegistration {
    ProjectionSubtypeRegistration()
    {
        auto & registry = reflection::TypeRegistry::get();
        registry.registerSubtype<math::Projection, math::PerspectiveProjection>(
            projectionType,
            "PerspectiveProjection",
            "Perspective",
            perspectiveType
        );
        registry.registerSubtype<math::Projection, math::OpticalProjection>(
            projectionType,
            "OpticalProjection",
            "Optical",
            opticalType
        );
    }
};

ProjectionSubtypeRegistration _projectionSubtypeRegistration;

}
