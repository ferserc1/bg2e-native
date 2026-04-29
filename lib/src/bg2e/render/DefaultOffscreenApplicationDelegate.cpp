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

#include <bg2e/render/DefaultOffscreenApplicationDelegate.hpp>
#include <bg2e/app/OffscreenApplication.hpp>
#include <bg2e/utils/TextureCache.hpp>
#include <bg2e/render/RendererBasicForward.hpp>

namespace bg2e::render
{

template <typename RendererT>
void DefaultOffscreenApplicationDelegate<RendererT>::initConfig(
    int argc, char *argv[],
    bg2e::app::OffscreenApplicationConfig & outConfig
) {
}

template <typename RendererT>
void DefaultOffscreenApplicationDelegate<RendererT>::init(
    Engine * engine,
    std::shared_ptr<vulkan::Image> colorImage,
    std::shared_ptr<vulkan::Image> depthImage
) {
    _engine = engine;
    _colorImage = colorImage;
    _depthImage = depthImage;

    _renderer = std::make_unique<RendererT>();
    _renderer->build(
        engine,
        _colorImage->extent2D(),
        _colorImage->format(),
        _depthImage->format(),
        VK_SAMPLE_COUNT_1_BIT,
        true
    );
}

template <typename RendererT>
void DefaultOffscreenApplicationDelegate<RendererT>::initFrameResources(vulkan::DescriptorSetAllocator * allocator)
{
    _renderer->initFrameResources(allocator);
    allocator->initPool();
}

template <typename RendererT>
void DefaultOffscreenApplicationDelegate<RendererT>::initScene()
{
    _renderer->initScene(createScene());
}

template <typename RendererT>
void DefaultOffscreenApplicationDelegate<RendererT>::resize(uint32_t width, uint32_t height)
{
    _renderer->resize(VkExtent2D(width, height));
}

template <typename RendererT>
void DefaultOffscreenApplicationDelegate<RendererT>::frame(
    float delta,
    [[maybe_unused]] uint32_t frameIndex,
    [[maybe_unused]] vulkan::FrameResources& frameResources
) {
    _renderer->update(delta);
}

template <typename RendererT>
bool DefaultOffscreenApplicationDelegate<RendererT>::render(
    VkCommandBuffer cmd,
    uint32_t frameIndex,
    vulkan::FrameResources & frameResources,
    VkImageLayout colorImageLayout,
    VkImageLayout & finalColorImageLayout
) {
    _renderer->draw(
        cmd,
        frameIndex,
        _colorImage.get(),
        _depthImage.get(),
        _depthImage.get(),
        frameResources
    );
    // The renderer->draw() function sets the image layout as color attachment optimal
    finalColorImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    return continueRendering();
}

template <typename RendererT>
void DefaultOffscreenApplicationDelegate<RendererT>::didRenderFrame(
    uint32_t frameIndex,
    double elapsedMs,
    VkImageLayout colorImageLayout
) {

}

template <typename RendererT>
void DefaultOffscreenApplicationDelegate<RendererT>::cleanup()
{
    _renderer->cleanup();
    bg2e::utils::TextureCache::destroy();
}

template class BG2E_API DefaultOffscreenApplicationDelegate<RendererBasicForward>;

}