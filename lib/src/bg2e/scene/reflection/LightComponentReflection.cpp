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
#include <bg2e/scene/LightComponent.hpp>

using namespace bg2e;

namespace {

reflection::TypeRegistration<scene::LightComponent> _lightComponentReflection(
    [](reflection::TypeInfoBuilder<scene::LightComponent>& t) {
        t.displayName("Light Source");

        // light() has const and non-const overloads => static_cast disambiguation.
        // The sub-object is edited in place through base::Light's own
        // reflected setters (registered under "bg2e::base::Light").
        t.object("light", "bg2e::base::Light",
                 static_cast<const base::Light&(scene::LightComponent::*)() const>(&scene::LightComponent::light),
                 static_cast<base::Light&(scene::LightComponent::*)()>(&scene::LightComponent::light))
            .displayName("Properties")
            .category("Light")
            .tooltip("Light parameters, edited in place");
    });

}
