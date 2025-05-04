
#pragma once

#include <bg2e/common.hpp>
#include <bg2e/base/Image.hpp>
#include <bg2e/render/Vulkan.hpp>
#include <bg2e/render/vulkan/geo/Mesh.hpp>
#include <bg2e/render/vulkan/Buffer.hpp>
#include <bg2e/render/Texture.hpp>
#include <bg2e/render/vulkan/FrameResources.hpp>
#include <filesystem>
#include <memory>

namespace bg2e {
namespace render {

class BG2E_API SphereToCubemapRenderer {
public:
    SphereToCubemapRenderer(Vulkan * vulkan);

    void initFrameResources(vulkan::DescriptorSetAllocator*);

    void build(
        const std::filesystem::path& imagePath,
        const std::string& vertexShader = "sphere_to_cubemap.vert.spv",
        const std::string& fragmentShader = "sphere_to_cubemap.frag.spv",
        VkExtent2D cubeImageSize = { 1024, 1024 }
    );
    
    void build(
        const std::filesystem::path& imagePath,
        VkExtent2D cubeImageSize
    ) {
        build(imagePath, "sphere_to_cubemap.vert.spv", "sphere_to_cubemap.frag.spv", cubeImageSize);
    }
    
    void updateImage(const std::filesystem::path& imagePath);
    
    void update(VkCommandBuffer commandBuffer, vulkan::FrameResources& frameResources);
    
    void cleanup();

    std::shared_ptr<vulkan::Image> cubeMapImage() { return _cubeMapImage; }
    
protected:
    Vulkan * _vulkan;

    std::shared_ptr<vulkan::geo::MeshPU> _sphere;
    float _sphereRadius = 10.0f;
    
    struct RenderSpherePushConstant
    {
        int currentFace;
        int currentMipLevel = 0;
        int totalMipLevels = 1;
    };
    
    struct ProjectionData
    {
        glm::mat4 view[6];
        glm::mat4 proj;
    };
    
    VkPipelineLayout _pipelineLayout;
    VkPipeline _pipeline;
    std::unique_ptr<vulkan::Buffer> _projectionDataBuffer;
    std::shared_ptr<Texture> _skyTexture;
    VkDescriptorSetLayout _dsLayout;
    ProjectionData _projectionData;
    
    std::shared_ptr<vulkan::Image> _cubeMapImage;
    VkImageView _cubeMapImageViews[6];
    
    void initImages(VkExtent2D);
    void initPipeline(const std::string& vshaderFile, const std::string& fshaderFile);
    void initGeometry();
    
};

}
}
