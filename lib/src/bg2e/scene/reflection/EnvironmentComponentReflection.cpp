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
#include <bg2e/scene/EnvironmentComponent.hpp>

using namespace bg2e;

namespace {

reflection::TypeRegistration<scene::EnvironmentComponent> _environmentReflection(
    [](reflection::TypeInfoBuilder<scene::EnvironmentComponent>& t)
    {
        t.displayName("Environment");
        t.property("skyHorizonColor", &scene::EnvironmentComponent::skyHorizonColor, &scene::EnvironmentComponent::setSkyHorizonColor)
            .displayName("Sky horizon color").category("Procedural sky").colorEditor()
            .tooltip("Linear RGB color used when no environment image is selected");

        t.property("skyZenithColor", &scene::EnvironmentComponent::skyZenithColor, &scene::EnvironmentComponent::setSkyZenithColor)
            .displayName("Sky zenith color").category("Procedural sky").colorEditor()
            .tooltip("Linear RGB color used when no environment image is selected");

        t.property("groundColor", &scene::EnvironmentComponent::groundColor, &scene::EnvironmentComponent::setGroundColor)
            .displayName("Ground color").category("Procedural sky").colorEditor()
            .tooltip("Linear RGB color used when no environment image is selected");

        t.property("skyboxVisible", &scene::EnvironmentComponent::skyboxVisible, &scene::EnvironmentComponent::setSkyboxVisible)
            .displayName("Sky visible").category("Environment").checkbox();
        t.property("skyboxBlur", &scene::EnvironmentComponent::skyboxBlur, &scene::EnvironmentComponent::setSkyboxBlur)
            .displayName("Sky blur").category("Environment").range(0, 1).slider();

        t.property(
             "environmentImage",
             &scene::EnvironmentComponent::environmentImage,
             static_cast<void (scene::EnvironmentComponent::*)(const std::string&)>(
             &scene::EnvironmentComponent::setEnvironmentImage))
            .displayName("Environment Image")
             .category("Environment")
             .tooltip("Equirectangular SDR (sRGB) or HDR image; SDR is converted to linear lighting")
             .resource("Environment Images", {"hdr", "jpg", "jpeg", "png", "bmp", "tga"})
             .resourceEmptySelection()
             .clearableResource();
    });

}
