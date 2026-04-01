/*
 *    business grade graphic engine (bg2 engine)
 *    Copyright (C) 2026  Fernando Serrano Carpena
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <bg2e/common.hpp>
#include <bg2e/render/Engine.hpp>
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
    CubemapRenderer(Engine *);
    
    void initFrameResources(vulkan::DescriptorSetAllocator*);
    
    void build(
        std::shared_ptr<vulkan::Image> inputSkyBox,
        const std::string& vshaderFile,
        const std::string& fshaderFile,
        VkExtent2D cubeImageSize = { 1024, 1024 },
        bool useMipmaps = false,
        uint32_t maxMipmapLevels = 20,
        VkFormat imageFormat = VK_FORMAT_R16G16B16A16_SFLOAT
    );
    
    void update(
        VkCommandBuffer commandBuffer,
        uint32_t currentFrame,
        vulkan::FrameResources& frameResources
    );
    
    std::shared_ptr<vulkan::Image> cubeMapImage() { return _cubeMapImage; }
    
protected:
    Engine * _engine;
    
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
    glm::mat4 _viewTransform = glm::mat4{ 1.0f };
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
        uint32_t maxMipmapLevels,
        VkFormat imageFormat = VK_FORMAT_R16G16B16A16_SFLOAT
    );
    
    void createPipelineLayout();
    
    void initPipeline(
        const std::string& vshaderFile,
        const std::string& fshaderFile
    );
    
    void initGeometry();
};

}
}
