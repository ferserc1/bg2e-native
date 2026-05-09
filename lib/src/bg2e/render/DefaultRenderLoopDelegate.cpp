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

#include <bg2e/render/DefaultRenderLoopDelegate.hpp>
#include <bg2e/render/ColorAttachments.hpp>
#include <bg2e/render/RendererBasicForward.hpp>
#include <bg2e/render/RendererDeferred.hpp>
#include <bg2e/utils/TextureCache.hpp>

namespace bg2e::render {

template <typename RendererT>
DefaultRenderLoopDelegate<RendererT>::~DefaultRenderLoopDelegate()
{

}

template <typename RendererT>
void DefaultRenderLoopDelegate<RendererT>::init(render::Engine * engine)
{
    RenderLoopDelegate::init(engine);

    _renderer = std::make_unique<RendererT>();
    _renderer->build(
        engine,
        engine->swapchain().extent(),
        engine->swapchain().imageFormat(),
        engine->swapchain().depthImageFormat(),
        engine->swapchain().sampleCount(),
        false
    );
}

template <typename RendererT>
void DefaultRenderLoopDelegate<RendererT>::initFrameResources(render::vulkan::DescriptorSetAllocator* frameAllocator)
{
    _renderer->initFrameResources(frameAllocator);
    frameAllocator->initPool();
}

template <typename RendererT>
void DefaultRenderLoopDelegate<RendererT>::initScene()
{
    _renderer->initScene(createScene());
}

template <typename RendererT>
void DefaultRenderLoopDelegate<RendererT>::swapchainResized(VkExtent2D newExtent)
{
    _renderer->resize(newExtent);
}

template <typename RendererT>
void DefaultRenderLoopDelegate<RendererT>::update(
    uint32_t /* currentFrame */,
    render::vulkan::FrameResources&
) {
    _renderer->update(delta());
}

template <typename RendererT>
VkImageLayout DefaultRenderLoopDelegate<RendererT>::render(
    VkCommandBuffer cmd,
    uint32_t currentFrame,
    const render::vulkan::Image* colorImage,
    const render::vulkan::Image* depthImage,
    const render::vulkan::Image* msaaDepthImage,
    render::vulkan::FrameResources& frameResources
) {
    _renderer->draw(
        cmd,
        currentFrame,
        colorImage,
        depthImage,
        msaaDepthImage,
        frameResources
    );

    return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
}

template <typename RendererT>
void DefaultRenderLoopDelegate<RendererT>::cleanup()
{
    _renderer->cleanup();
    bg2e::utils::TextureCache::destroy();
}

template <typename RendererT>
RendererT* DefaultRenderLoopDelegate<RendererT>::renderer()
{
    return _renderer.get();
}

template class BG2E_API DefaultRenderLoopDelegate<RendererBasicForward>;
template class BG2E_API DefaultRenderLoopDelegate<RendererDeferred>;

}
