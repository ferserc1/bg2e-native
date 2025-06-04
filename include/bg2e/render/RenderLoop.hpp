#pragma once

#include <bg2e/common.hpp>
#include <bg2e/render/Engine.hpp>
#include <bg2e/render/vulkan/DescriptorSetAllocator.hpp>
#include <bg2e/render/RenderLoopDelegate.hpp>

#include <memory>
#include <functional>

namespace bg2e {
namespace render {

class BG2E_API RenderLoop {
public:
    // Init engine resources, for example, the main descriptor set allocator,
    // render images, etc
    void init(Engine * engine);
    
    // Init scene resource, for example: create pipelines, load textures,
    // load 3d models...
    void initScene();

    void acquireAndPresent();

    void swapchainResized();

    void initFrameResources(vulkan::DescriptorSetAllocator*);

    VkImageLayout render(
        VkCommandBuffer cmd,
        const vulkan::Image* colorImage,
        const vulkan::Image* depthImage,
        vulkan::FrameResources& frameResources
    );

    void cleanup();

    inline void setDelegate(std::shared_ptr<RenderLoopDelegate> delegate) { _renderDelegate = delegate; }

	inline void renderUICallback(std::function<void(VkCommandBuffer, VkImageView)> callback) { _renderUICallback = callback; }

    inline void setDelta(float d) { _delta = d; }
    
protected:
	Engine * _engine;
    bool _resizeRequested = false;

    std::shared_ptr<RenderLoopDelegate> _renderDelegate;

    std::function<void(VkCommandBuffer, VkImageView)> _renderUICallback;
    
    float _delta;
};

}
}
