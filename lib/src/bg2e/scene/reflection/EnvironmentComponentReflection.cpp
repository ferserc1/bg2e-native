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
#include <bg2e/scene/EnvironmentComponent.hpp>

using namespace bg2e;

namespace {

reflection::TypeRegistration<scene::EnvironmentComponent> _environmentReflection(
    [](reflection::TypeInfoBuilder<scene::EnvironmentComponent>& t) {
        t.displayName("Environment");
        t.property(
            "environmentImage",
            &scene::EnvironmentComponent::environmentImage,
            static_cast<void(scene::EnvironmentComponent::*)(const std::string&)>(
                &scene::EnvironmentComponent::setEnvironmentImage
            )
        )
            .displayName("Environment Image")
            .category("Environment")
            .tooltip("Equirectangular HDR or image used for environment lighting")
            .resource("Environment Images", { "hdr", "exr", "jpg", "jpeg", "png", "bmp", "webp" });
    });

}
