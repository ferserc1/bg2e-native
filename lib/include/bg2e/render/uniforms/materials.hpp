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

#pragma once

#include <bg2e/math/base.hpp>
#include <bg2e/base/Color.hpp>

namespace bg2e {
namespace render {
namespace uniforms {

struct PBRMaterialData
{
    enum Flags : uint32_t
    {
        NONE = 0x0,
        UNLIT = 0x1u << 0,
    };

    bg2e::base::Color albedo;  // Base color of the material
    bg2e::base::Color fresnelTint;

    glm::vec2 albedoScale;  // Scale for albedo texture
    glm::vec2 normalScale;  // Scale for normal texture
    glm::vec2 metalnessScale;  // Scale for metalness texture
    glm::vec2 roughnessScale;  // Scale for roughness texture

    float metalness;
    float roughness;

    uint32_t albedoUVSet;  // UV set for albedo texture
    uint32_t normalUVSet;  // UV set for normal texture
    uint32_t metalnessUVSet;  // UV set for metalness texture
    uint32_t roughnessUVSet;  // UV set for roughness texture
    uint32_t aoUVSet;  // UV set for ambient occlusion texture
    
    float sheenIntensity;
    bg2e::base::Color sheenColor;

    uint32_t flags;
    uint32_t padding[3] = {};
    
    void operator=(const base::MaterialAttributes& att)
    {
        albedo = att.albedo();
        fresnelTint = att.fresnelTint();
        albedoScale = att.albedoScale();
        normalScale = att.normalScale();
        metalnessScale = att.metalnessScale();
        roughnessScale = att.roughnessScale();
        metalness = att.metalness();
        roughness = att.roughness();
        albedoUVSet = att.albedoUVSet();
        normalUVSet = att.normalUVSet();
        metalnessUVSet = att.metalnessUVSet();
        roughnessUVSet = att.roughnessUVSet();
        aoUVSet = att.aoUVSet();
        sheenIntensity = att.sheenIntensity();
        sheenColor = att.sheenColor();

        flags = 0;
        flags |= att.isUnlit() ? 0x1 : 0x0;
    }
};

}
}
}
