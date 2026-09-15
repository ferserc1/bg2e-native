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
#include <bg2e/base/Light.hpp>

using namespace bg2e;

namespace {

reflection::TypeRegistration<base::Light> _lightReflection("bg2e::base::Light",
    [](reflection::TypeInfoBuilder<base::Light>& t) {
        t.displayName("Light");

        t.property("color", &base::Light::color, &base::Light::setColor)
            .displayName("Color")
            .category("Light")
            .colorEditor();

        t.property("intensity", &base::Light::intensity, &base::Light::setIntensity)
            .displayName("Intensity")
            .category("Light")
            .range(0.0, 100.0)
            .slider()
            .step(0.1);

        t.property("type", &base::Light::type, &base::Light::setType)
            .displayName("Type")
            .category("Light")
            .combo()
            .enumValue("Omni", base::Light::TypeOmni)
            .enumValue("Spot", base::Light::TypeSpot)
            .enumValue("Directional", base::Light::TypeDirectional)
            .enumValue("Disabled", base::Light::TypeDisabled);

        t.property("spotAngle", &base::Light::spotAngle, &base::Light::setSpotAngle)
            .displayName("Spot Angle")
            .category("Spot")
            .range(0.0, 90.0)
            .angle();

        t.property("spotCutoff", &base::Light::spotCutoff, &base::Light::setSpotCutoff)
            .displayName("Spot Cutoff")
            .category("Spot")
            .range(0.0, 90.0)
            .angle();

        t.property("castShadows", &base::Light::castShadows, &base::Light::setCastShadows)
            .displayName("Cast Shadows")
            .category("Shadows")
            .checkbox();

        t.property("shadowSamples", &base::Light::shadowSamples, &base::Light::setShadowSamples)
            .displayName("Shadow Samples")
            .category("Shadows")
            .range(1.0, 64.0)
            .step(1.0);

        t.property("sourceSize", &base::Light::sourceSize, &base::Light::setSourceSize)
            .displayName("Source Size")
            .category("Light")
            .range(0.0, 10.0)
            .drag()
            .step(0.01);

        t.property("affectsReflections", &base::Light::affectsReflections,
                                         &base::Light::setAffectsReflections)
            .displayName("Affects Reflections")
            .category("Light")
            .checkbox();

        // Getter-only => read-only (no setter registered).
        t.property("typeString", &base::Light::typeString)
            .displayName("Type (string)")
            .category("Light");
    });

}
