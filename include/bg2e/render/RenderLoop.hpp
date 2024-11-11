#pragma once

#include <bg2e/common.hpp>
#include <bg2e/render/Vulkan.hpp>
#include <bg2e/render/vulkan/DescriptorSetAllocator.hpp>
#include <bg2e/render/RenderLoopDelegate.hpp>

#include <memory>

namespace bg2e {
namespace render {

class BG2E_API RenderLoop {
public:
    void init(Vulkan * vulkan);

    void acquireAndPresent();

    void swapchainResized();

    void initFrameResources(vulkan::DescriptorSetAllocator*);

    VkImageLayout render(
        VkCommandBuffer cmd,
        const vulkan::Image* colorImage,
        const vulkan::Image* depthImage,
        vulkan::FrameResources& frameResources
    );

    inline void setDelegate(std::shared_ptr<RenderLoopDelegate> delegate) { _renderDelegate = delegate; }

protected:
	Vulkan* _vulkan;
    bool _resizeRequested = false;

    std::shared_ptr<RenderLoopDelegate> _renderDelegate;

};

}
}
