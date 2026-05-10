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

#include <bg2e/render/deferred/RenderLayer.hpp>

namespace bg2e {
namespace render {
namespace deferred {

class BG2E_API SkyboxLayer : public RenderLayer {
public:
    SkyboxLayer(Engine* engine);
    ~SkyboxLayer() override;

    void build(VkExtent2D extent, VkFormat outputFormat) override;
    void render(
        VkCommandBuffer cmd,
        uint32_t currentFrame,
        const vulkan::Image* inputImage,
        const vulkan::Image* outputImage,
        vulkan::FrameResources& frameResources
    ) override;
    void resize(VkExtent2D newExtent) override;
    void cleanup() override;

    [[nodiscard]] bool drawSkybox() const { return _drawSkybox; }
    void setDrawSkybox(bool drawSkybox) { _drawSkybox = drawSkybox; }

protected:
    bool _drawSkybox = true;
};

}
}
}
