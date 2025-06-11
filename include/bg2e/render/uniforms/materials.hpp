#pragma once

#include <bg2e/math/base.hpp>
#include <bg2e/base/Color.hpp>

namespace bg2e {
namespace render {
namespace uniforms {

struct PBRMaterialData
{
    bg2e::base::Color albedo;  // Base color of the material

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
};

}
}
}