
#pragma once

#include <bg2e/render/CubemapRenderer.hpp>

namespace bg2e {
namespace render {

class BG2E_API IrradianceCubemapRenderer : public CubemapRenderer {
public:
    IrradianceCubemapRenderer(Engine * engine);
    
    void initFrameResources(vulkan::DescriptorSetAllocator*);
    
    void build(
        std::shared_ptr<vulkan::Image> inputCubemap,
        VkExtent2D cubeImageSize = { 128, 128 }
    );
};

}
}
