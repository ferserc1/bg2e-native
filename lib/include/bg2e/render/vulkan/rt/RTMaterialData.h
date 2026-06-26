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

#include <bg2e/base/Color.hpp>
#include <bg2e/math/base.hpp>
#include <bg2e/render/vulkan/Buffer.hpp>
#include <bg2e/render/Texture.hpp>

#include <memory>

namespace bg2e {
namespace render {
namespace vulkan {
namespace rt {

static constexpr uint32_t MAX_OBJECTS = 256;

struct RTMaterialData {
    base::Color albedo;
    glm::vec2 albedoScale;
    uint32_t indexOffset;   // firstIndex of the submesh in the shared index buffer
    float lightEmission;
    uint32_t lightEmissionChannel;
    uint32_t lightEmissionInvert;
    glm::vec2 lightEmissionScale;
    uint32_t lightEmissionUVSet;
    uint32_t padding;
};

struct RTMaterialInstance {
    RTMaterialData data;
    const Buffer* vertexBuffer = nullptr;
    const Buffer* indexBuffer = nullptr;
    std::shared_ptr<render::Texture> albedoTexture;
    std::shared_ptr<render::Texture> lightEmissionTexture;
};

struct RTObjectInstance {
    RTMaterialData materialData;
    const Buffer*  vertexBuffer = nullptr;
    const Buffer*  indexBuffer = nullptr;
    std::shared_ptr<render::Texture> albedoTexture;
    std::shared_ptr<render::Texture> lightEmissionTexture;
};

}
}
}
}