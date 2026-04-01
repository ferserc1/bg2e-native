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
#include <bg2e/base/Image.hpp>
#include <bg2e/render/Engine.hpp>
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
    SphereToCubemapRenderer(Engine * engine);
    virtual ~SphereToCubemapRenderer();

    void initFrameResources(vulkan::DescriptorSetAllocator*);

    void build(
        const std::filesystem::path& imagePath,
        const std::string& vertexShader = "sphere_to_cubemap.vert.spv",
        const std::string& fragmentShader = "sphere_to_cubemap.frag.spv",
        VkExtent2D cubeImageSize = { 1024, 1024 }
    );
    
    void build(
        std::shared_ptr<render::Texture> texture,
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
    
    void build(
        std::shared_ptr<render::Texture> texture,
        VkExtent2D cubeImageSize = { 1024, 1024 }
    ) {
        build(texture, "sphere_to_cubemap.vert.spv", "sphere_to_cubemap.frag.spv", cubeImageSize);
    }
    
    void updateImage(const std::filesystem::path& imagePath);
    void updateImage(std::shared_ptr<render::Texture> texture);
    
    void update(VkCommandBuffer commandBuffer, vulkan::FrameResources& frameResources);

    std::shared_ptr<vulkan::Image> cubeMapImage() { return _cubeMapImage; }
    
protected:
    Engine * _engine;

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
    
    // Called by destructor
    void cleanup();
    
};

}
}
