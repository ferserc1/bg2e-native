#pragma once

#include <bg2e/render/CubemapRenderer.hpp>

namespace bg2e {
namespace render {

class BG2E_API SkyboxRenderer : public CubemapRenderer {
public:
    SkyboxRenderer(Vulkan *);
    
    void initFrameResources(vulkan::DescriptorSetAllocator*);
    
    void build(
        std::shared_ptr<Texture> skyTexture,
        VkFormat colorAttachmentFormat,
        VkFormat depthAttachmentFormat,
        float cubeSize = 10.0f
    );
};

}
}
