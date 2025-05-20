#pragma once

#include <bg2e/render/CubemapRenderer.hpp>

#include <vector>

namespace bg2e {
namespace render {

class BG2E_API SkyboxRenderer {
public:
    SkyboxRenderer(Vulkan *);
    virtual ~SkyboxRenderer();
    
    void initFrameResources(vulkan::DescriptorSetAllocator*);
    
    void build(
        std::shared_ptr<Texture> skyTexture,
        const std::vector<VkFormat>& colorAttachmentFormat,
        VkFormat depthAttachmentFormat,
        const std::string& vshaderFile = "skybox_renderer.vert.spv",
        const std::string& fshaderFile = "skybox_renderer.frag.spv",
        float cubeSize = 10.0f
    );
    
    void update(const glm::mat4& view, const glm::mat4& proj);
    
    void draw(
        VkCommandBuffer commandBuffer,
        uint32_t currentFrame,
        vulkan::FrameResources& frameResources
    );
    
protected:
    Vulkan * _vulkan;
    
    VkDescriptorSetLayout _dsLayout;
    VkPipelineLayout _pipelineLayout;
    VkPipeline _pipeline;
    
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
