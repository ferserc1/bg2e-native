#pragma once

#include <bg2e/render/CubemapRenderer.hpp>
#include <bg2e/base/PlatformTools.hpp>

#include <vector>

namespace bg2e {
namespace render {

class BG2E_API SkyboxRenderer {
public:
    SkyboxRenderer(Engine *);
    virtual ~SkyboxRenderer();
    
    void initFrameResources(vulkan::DescriptorSetAllocator*);
    
    inline void setSampleCount(VkSampleCountFlagBits samples) { _samples = samples; }
    inline VkSampleCountFlagBits sampleCount() const { return _samples; }
    
    void build(
        std::shared_ptr<Texture> skyTexture,
        const std::vector<VkFormat>& colorAttachmentFormat,
        VkFormat depthAttachmentFormat,
        const std::string& vshaderFile = "skybox_renderer.vert.spv",
#ifdef BG2E_IS_MAC
        const std::string& fshaderFile = "skybox_renderer_metal.frag.spv",
#else
		const std::string& fshaderFile = "skybox_renderer.frag.spv",
#endif
        float cubeSize = 10.0f
    );
    
    void update(const glm::mat4& view, const glm::mat4& proj);
    
    void draw(
        VkCommandBuffer commandBuffer,
        uint32_t currentFrame,
        vulkan::FrameResources& frameResources
    );
    
protected:
    Engine * _engine;
    
    VkDescriptorSetLayout _dsLayout = VK_NULL_HANDLE;
    VkPipelineLayout _pipelineLayout = VK_NULL_HANDLE;
    VkPipeline _pipeline = VK_NULL_HANDLE;
    VkSampleCountFlagBits _samples = VK_SAMPLE_COUNT_1_BIT;
    
    struct SkyData {
        glm::mat4 view;
        glm::mat4 proj;
    };
    SkyData _skyData;
    
    std::shared_ptr<Texture> _skyTexture;
    std::shared_ptr<vulkan::geo::MeshP> _cube;
    
    // Called by destructor
    void cleanup();
};

}
}
