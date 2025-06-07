#pragma once

#include <bg2e/render/RenderLoopDelegate.hpp>
#include <bg2e/render/RendererBasicForward.hpp>
#include <bg2e/scene/Node.hpp>

#include <memory>

namespace bg2e {
namespace render {

template<typename RendererT>
class BG2E_API DefaultRenderLoopDelegate : public RenderLoopDelegate {
public:
    virtual ~DefaultRenderLoopDelegate() = 0;

    void init(render::Engine * engine) override;

    void initFrameResources(render::vulkan::DescriptorSetAllocator*) override;
    
    void initScene() override;

    void swapchainResized(VkExtent2D) override;

    void update(
        uint32_t currentFrame,
        render::vulkan::FrameResources&
    )override;

    VkImageLayout render(
        VkCommandBuffer cmd,
        uint32_t currentFrame,
        const render::vulkan::Image* colorImage,
        const render::vulkan::Image* depthImage,
        render::vulkan::FrameResources& frameResources
    ) override;

	void cleanup() override;


    inline RendererT * renderer() { return _renderer.get(); }
    
protected:
	std::unique_ptr<RendererT> _renderer;

    virtual std::shared_ptr<scene::Node> createScene() = 0;
};

}
}
