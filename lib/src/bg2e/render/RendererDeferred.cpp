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

#include <bg2e/render/RendererDeferred.hpp>
#include <bg2e/render/vulkan/macros/graphics.hpp>

namespace bg2e::render {

void RendererDeferred::build(
    bg2e::render::Engine* engine,
    VkExtent2D initialExtent,
    VkFormat colorImageFormat,
    VkFormat depthImageFormat,
    VkSampleCountFlagBits sampleCount,
    bool isOffscreen
) {
    _engine = engine;
    _viewportExtent = initialExtent;
    _colorImageFormat = colorImageFormat;
    _depthImageFormat = depthImageFormat;
    _sampleCount = sampleCount;
    _isOffscreen = isOffscreen;

    _scene = std::make_unique<bg2e::scene::Scene>();
}

void RendererDeferred::initFrameResources(
    bg2e::render::vulkan::DescriptorSetAllocator* /*frameAllocator*/
) {
}

void RendererDeferred::initScene(
    std::shared_ptr<bg2e::scene::Node> sceneRoot
) {
    _scene->setSceneRoot(sceneRoot);
}

void RendererDeferred::resize(
    VkExtent2D newExtent
) {
    _viewportExtent = newExtent;
}

void RendererDeferred::update(
    float /*delta*/
) {
    _scene->willUpdate();
    _scene->didUpdate();
}

void RendererDeferred::draw(
    VkCommandBuffer cmd,
    uint32_t /*currentFrame*/,
    const bg2e::render::vulkan::Image* colorImage,
    const bg2e::render::vulkan::Image* /*depthImage*/,
    const bg2e::render::vulkan::Image* /*msaaDepthImage*/,
    bg2e::render::vulkan::FrameResources& /*frameResources*/
) {
    VkClearColorValue clearValue{ { 0.1f, 0.1f, 0.2f, 1.0f } };
    vulkan::macros::cmdClearImageAndBeginRendering(cmd, colorImage, clearValue);
    vulkan::macros::cmdSetDefaultViewportAndScissor(cmd, colorImage->extent2D());
    vulkan::cmdEndRendering(cmd);
}

void RendererDeferred::cleanup() {
    _scene.reset();
}

bg2e::scene::Scene* RendererDeferred::scene() {
    return _scene.get();
}

}
