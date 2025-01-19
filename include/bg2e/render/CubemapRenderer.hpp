#pragma once

#include <bg2e/common.hpp>
#include <bg2e/render/Vulkan.hpp>
#include <bg2e/render/Texture.hpp>
#include <bg2e/render/vulkan/Buffer.hpp>
#include <bg2e/render/vulkan/DescriptorSetAllocator.hpp>
#include <bg2e/render/vulkan/geo/Mesh.hpp>

#include <vector>
#include <memory>

namespace bg2e {
namespace render {

class BG2E_API CubemapRenderer {
public:
    CubemapRenderer(Vulkan *);
    
    void initFrameResources(vulkan::DescriptorSetAllocator*);
    
    void build(
        std::shared_ptr<vulkan::Image> inputSkyBox,
        const std::string& vshaderFile,
        const std::string& fshaderFile,
        VkExtent2D cubeImageSize = { 1024, 1024 },
        bool useMipmaps = false,
        uint32_t maxMipmapLevels = 20
    );
    
    void update(
        VkCommandBuffer commandBuffer,
        uint32_t currentFrame,
        vulkan::FrameResources& frameResources
    );
    
    std::shared_ptr<vulkan::Image> cubeMapImage() { return _cubeMapImage; }
    
protected:
    Vulkan * _vulkan;
    
    std::shared_ptr<vulkan::Image> _inputSkybox;
    VkSampler _skyImageSampler;
    
    struct MipLevelImageViews
    {
        VkImageView imageViews[6];
    };
    std::shared_ptr<vulkan::Image> _cubeMapImage;
    std::vector<MipLevelImageViews> _cubeMapImageViews;
    
    struct ProjectionData
    {
        glm::mat4 view[6];
        glm::mat4 proj;
    };
    ProjectionData _projectionData;
    std::unique_ptr<vulkan::Buffer> _projectionDataBuffer;
    
    struct SkyPushConstants {
        int currentFace;
        int currentMipLevel;
        int totalMipLevels;
    };
    
    VkDescriptorSetLayout _descriptorSetLayout;
    VkPipelineLayout _layout;
    VkPipeline _pipeline;
    
    
    std::unique_ptr<vulkan::geo::MeshP> _cube;
    
    void initImages(
        VkExtent2D cubeImageSize,
        bool useMipmaps,
        uint32_t maxMipmapLevels
    );
    
    void initPipeline(
        const std::string& vshaderFile,
        const std::string& fshaderFile
    );
    
    void initGeometry();
};

}
}
