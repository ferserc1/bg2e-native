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

#include <bg2e/render/deferred/SkyboxLayer.hpp>
#include <bg2e/render/vulkan/macros/graphics.hpp>
#include <bg2e/render/vulkan/extensions.hpp>

namespace bg2e::render::deferred {

SkyboxLayer::SkyboxLayer(Engine* engine)
    : RenderLayer { engine }
{
}

SkyboxLayer::~SkyboxLayer()
{
}

void SkyboxLayer::build(VkExtent2D extent, VkFormat outputFormat)
{
    RenderLayer::build(extent, outputFormat);
}

void SkyboxLayer::render(
    VkCommandBuffer cmd,
    uint32_t currentFrame,
    const vulkan::Image* /*inputImage*/,
    const vulkan::Image* outputImage,
    vulkan::FrameResources& frameResources
)
{
    if (!_environment || !_scene)
    {
        return;
    }

    VkClearColorValue clearValue{ { 0.0f, 0.0f, 0.0f, 1.0f } };
    vulkan::macros::cmdClearImageAndBeginRendering(cmd, outputImage, clearValue);

    vulkan::macros::cmdSetDefaultViewportAndScissor(cmd, _extent);

    auto mainCamera = _scene->mainCamera();
    if (mainCamera)
    {
        auto viewMatrix = mainCamera->ownerNode()->invertedWorldMatrix();
        auto projMatrix = mainCamera->projectionMatrix();

        _environment->updateSkybox(viewMatrix, projMatrix);
        _environment->drawSkybox(cmd, currentFrame, frameResources);
    }

    vulkan::cmdEndRendering(cmd);
}

void SkyboxLayer::resize(VkExtent2D newExtent)
{
    RenderLayer::resize(newExtent);
}

void SkyboxLayer::cleanup()
{
}

}
