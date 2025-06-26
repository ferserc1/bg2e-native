#pragma once

#include <bg2e/render/Engine.hpp>
#include <bg2e/render/vulkan/Image.hpp>
#include <bg2e/render/vulkan/DescriptorSetAllocator.hpp>

namespace bg2e {
namespace render {

class RenderLoopDelegate {
public:
    virtual ~RenderLoopDelegate() {}

    // Init engine resource, for example, the main descriptor set allocator requirements
    virtual void init(render::Engine * engine)
    {
		_engine = engine;
    }

    // Init frame resources, tipically, the frame descriptor set allocator
    virtual void initFrameResources(render::vulkan::DescriptorSetAllocator*) {}
    
    // Init scene data, such as pipelines, textures or 3D models
    virtual void initScene() {}

    virtual void swapchainResized(VkExtent2D) = 0;

    virtual void update(uint32_t currentFrame, render::vulkan::FrameResources&) {}

    virtual VkImageLayout render(
        VkCommandBuffer cmd,
        uint32_t currentFrame,
        const render::vulkan::Image* colorImage,
        const render::vulkan::Image* depthImage,
        const render::vulkan::Image* msaaDepthImage,
        render::vulkan::FrameResources& frameResources
    ) = 0;

	virtual void cleanup() {}

	inline render::Engine * engine() { return _engine; }

    inline float delta() const { return _delta; }
    inline void setDelta(float d) { _delta = d; }

protected:
	render::Engine * _engine = nullptr;
    
    float _delta = 0.0f;
};

}
}
