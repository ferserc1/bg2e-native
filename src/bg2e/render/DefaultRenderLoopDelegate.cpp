
#include <bg2e/render/DefaultRenderLoopDelegate.hpp>
#include <bg2e/render/ColorAttachments.hpp>
#include <bg2e/render/RendererBasicForward.hpp>
#include <bg2e/utils/TextureCache.hpp>

namespace bg2e::render {

template <typename RendererT>
DefaultRenderLoopDelegate<RendererT>::~DefaultRenderLoopDelegate<RendererT>()
{

}

template <typename RendererT>
void DefaultRenderLoopDelegate<RendererT>::init(render::Engine * engine)
{
    RenderLoopDelegate::init(engine);
    
    _renderer = std::make_unique<RendererT>();
    // TODO: allow RendererT to specify color and depth formats
    _renderer->build(engine);
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
    uint32_t currentFrame,
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
        depthImage,
        msaaDepthImage,
        frameResources
    );

    vulkan::Image::cmdCopy(
        cmd,
        _renderer->colorAttachments()->imageWithIndex(0)->handle(),
        _renderer->colorAttachments()->extent(),
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        colorImage->handle(), colorImage->extent2D(),
        VK_IMAGE_LAYOUT_UNDEFINED
    );

    return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
}

template <typename RendererT>
void DefaultRenderLoopDelegate<RendererT>::cleanup()
{
    _renderer->cleanup();
    bg2e::utils::TextureCache::destroy();
}

template DefaultRenderLoopDelegate<RendererBasicForward>::~DefaultRenderLoopDelegate<RendererBasicForward>();
template void DefaultRenderLoopDelegate<RendererBasicForward>::init(render::Engine * engine);
template void DefaultRenderLoopDelegate<RendererBasicForward>::initFrameResources(render::vulkan::DescriptorSetAllocator*);
template void DefaultRenderLoopDelegate<RendererBasicForward>::initScene();
template void DefaultRenderLoopDelegate<RendererBasicForward>::swapchainResized(VkExtent2D);
template void DefaultRenderLoopDelegate<RendererBasicForward>::update(uint32_t currentFrame, render::vulkan::FrameResources&);
template VkImageLayout DefaultRenderLoopDelegate<RendererBasicForward>::render(VkCommandBuffer cmd, uint32_t currentFrame, const render::vulkan::Image* colorImage, const render::vulkan::Image* depthImage, const render::vulkan::Image* msaaDepthImage, render::vulkan::FrameResources& frameResources);
template void DefaultRenderLoopDelegate<RendererBasicForward>::cleanup();

}
