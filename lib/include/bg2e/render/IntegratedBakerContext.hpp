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

#include <bg2e/render/BakerContext.hpp>
#include <bg2e/render/LightmapBaker.hpp>
#include <bg2e/render/vulkan/FrameResources.hpp>

#include <memory>

namespace bg2e {
namespace scene {
class Node;
}
namespace render {

class BG2E_API IntegratedLightmapBaker final : public LightmapBaker {
public:
    void update(VkCommandBuffer cmd, vulkan::FrameResources& frameResources);

private:
    friend class IntegratedBakerContext;
    IntegratedLightmapBaker(std::shared_ptr<BakerContext> context,
                            std::shared_ptr<scene::Node> targetNode,
                            LightmapSettings settings);
};

class BG2E_API IntegratedBakerContext final : public BakerContext {
public:
    IntegratedBakerContext(Engine* engine, scene::Node* rootNode);

    [[nodiscard]] std::unique_ptr<IntegratedLightmapBaker> createBaker(
        std::shared_ptr<scene::Node> targetNode,
        LightmapSettings settings = {});
};

}
}
