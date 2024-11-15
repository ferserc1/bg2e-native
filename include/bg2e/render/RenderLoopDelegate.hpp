#pragma once

#include <bg2e/render/Vulkan.hpp>
#include <bg2e/render/vulkan/Image.hpp>
#include <bg2e/render/vulkan/DescriptorSetAllocator.hpp>

namespace bg2e {
namespace render {

class RenderLoopDelegate {
public:
    virtual ~RenderLoopDelegate() {}

    virtual void init(render::Vulkan* vulkan)
    {
		_vulkan = vulkan;
    }

    virtual void initFrameResources(render::vulkan::DescriptorSetAllocator*) {}

    virtual void swapchainResized(VkExtent2D) = 0;

    virtual void update(uint32_t currentFrame, render::vulkan::FrameResources&) {}

    virtual VkImageLayout render(
        VkCommandBuffer cmd,
        uint32_t currentFrame,
        const render::vulkan::Image* colorImage,
        const render::vulkan::Image* depthImage,
        render::vulkan::FrameResources& frameResources
    ) = 0;

	virtual void cleanup() {}

	inline render::Vulkan* vulkan() { return _vulkan; }

protected:
	render::Vulkan* _vulkan = nullptr;
};

}
}
