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
		const std::string& fshaderFile = "skybox_renderer.frag.spv",
        float cubeSize = 10.0f
    );
    
    void update(const glm::mat4& view, const glm::mat4& proj);
    
    void draw(
        VkCommandBuffer commandBuffer,
        uint32_t currentFrame,
        vulkan::FrameResources& frameResources
    );

    inline float brightness() const { return _pushConstants.brightness; }
    inline float contrast() const { return _pushConstants.contrast; }
    inline float exposure() const { return _pushConstants.exposure; }
    inline void setBrightness(float brightness) { _pushConstants.brightness = brightness; }
    inline void setContrast(float contrast) { _pushConstants.contrast = contrast; }
    inline void setExposure(float exposure) { _pushConstants.exposure = exposure; }
    
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

    struct PushConstants
    {
        float brightness = 0.0f;
        float contrast = 1.0f;
        float exposure = 1.0f;
    };

    PushConstants _pushConstants;
    
    // Called by destructor
    void cleanup();
};

}
}
