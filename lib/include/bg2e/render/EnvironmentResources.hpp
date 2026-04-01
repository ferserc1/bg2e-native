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

#include <bg2e/render/SphereToCubemapRenderer.hpp>
#include <bg2e/render/IrradianceCubemapRenderer.hpp>
#include <bg2e/render/SpecularReflectionCubemapRenderer.hpp>
#include <bg2e/render/SkyboxRenderer.hpp>

#include <memory>

namespace bg2e {
namespace render {

class BG2E_API EnvironmentResources {
public:
    EnvironmentResources(bg2e::render::Engine*);
    EnvironmentResources(bg2e::render::Engine*, const std::vector<VkFormat>& targetImages, VkFormat depthFormat, VkSampleCountFlagBits sampleCount);
    
    void initFrameResources(bg2e::render::vulkan::DescriptorSetAllocator *frameAllocator);
        
    void build(
        const std::filesystem::path& environmentTexture,
        VkExtent2D cubeMapSize = { 2048, 2048 },
        VkExtent2D irradianceMapSize = { 32, 32 },
        VkExtent2D specularReflectionSize = { 512, 512 }
    );
    
    void build(
        std::shared_ptr<render::Texture> texture,
        VkExtent2D cubeMapSize = { 2048, 2048 },
        VkExtent2D irradianceMapSize = { 32, 32 },
        VkExtent2D specularReflectionSize = { 512, 512 }
    );
    
    void swapEnvironmentTexture(const std::filesystem::path& environmentTexture);
    void swapEnvironmentTexture(std::shared_ptr<render::Texture> texture);
    void setSkyboxBlurLevel(int blurLevel);
    int skyboxBlurLevel() { return _skyboxBlurLevel; }
    
    void update(
        VkCommandBuffer cmd,
        uint32_t currentFrame,
        vulkan::FrameResources& frameResources
    );
    
    void updateSkybox(const glm::mat4& viewMatrix, const glm::mat4& projMatrix);
    void drawSkybox(
        VkCommandBuffer cmd,
        uint32_t currentFrame,
        vulkan::FrameResources& frameResources
    );
    
    std::shared_ptr<vulkan::Image> cubeMapImage() { return _sphereToCubemap->cubeMapImage(); }
    std::shared_ptr<vulkan::Image> irradianceMapImage() { return _irradianceRenderer->cubeMapImage(); }
    std::shared_ptr<vulkan::Image> specularReflectionMapImage() { return _specularRenderer->cubeMapImage(); }
    std::shared_ptr<render::Texture> brdfIntegrationMapImage() { return _brdfIntegrationMap; }

    inline VkSampler cubeMapSampler() const { return _cubeMapSampler; }
    inline VkSampler irradianceMapSampler() const { return _irradianceMapSampler; }
    inline VkSampler specularReflectionMapSampler() const { return _specularReflectionSampler; }
    inline VkSampler brdfIntegrationMapSampler() const { return _brdfIntegrationMapSampler; }
    
    inline void setSkyboxColorCorrection(
        float brightness,
        float contrast,
        float exposure
    ) const {
        if (_skyboxRenderer)
        {
            _skyboxRenderer->setBrightness(brightness);
            _skyboxRenderer->setContrast(contrast);
            _skyboxRenderer->setExposure(exposure);
        }
    }

protected:
    bg2e::render::Engine * _engine;
    
    std::shared_ptr<bg2e::render::Texture> _cubeMapTexture;
    std::unique_ptr<SphereToCubemapRenderer> _sphereToCubemap;
    std::unique_ptr<IrradianceCubemapRenderer> _irradianceRenderer;
    std::unique_ptr<SpecularReflectionCubemapRenderer> _specularRenderer;
    std::shared_ptr<bg2e::render::Texture> _brdfIntegrationMap;
    bool _cubemapChanged = true;
    bool _blurChanged = false;
    uint32_t _skyboxBlurLevel = 0;
    
    std::vector<VkFormat> _targetImagesFormat;
    VkFormat _depthImageFormat = VK_FORMAT_D16_UNORM_S8_UINT;
    std::unique_ptr<bg2e::render::SkyboxRenderer> _skyboxRenderer;
    
    VkSampler _cubeMapSampler = VK_NULL_HANDLE;
    VkSampler _irradianceMapSampler = VK_NULL_HANDLE;
    VkSampler _specularReflectionSampler = VK_NULL_HANDLE;
    VkSampler _brdfIntegrationMapSampler = VK_NULL_HANDLE;
    
    void buildBRDF();
};

}
}
