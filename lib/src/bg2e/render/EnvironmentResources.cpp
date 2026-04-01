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

#include <bg2e/render/EnvironmentResources.hpp>
#include <bg2e/render/vulkan/factory/Sampler.hpp>
#include <bg2e/render/BRDFIntegrationMapTextureGenerator.hpp>

namespace bg2e {
namespace render {

EnvironmentResources::EnvironmentResources(bg2e::render::Engine * engine)
    :_engine(engine)
{
    _sphereToCubemap = std::unique_ptr<SphereToCubemapRenderer>(
        new SphereToCubemapRenderer(_engine)
    );
    _irradianceRenderer = std::unique_ptr<IrradianceCubemapRenderer>(
        new IrradianceCubemapRenderer(_engine)
    );
    _specularRenderer = std::unique_ptr<SpecularReflectionCubemapRenderer>(
        new SpecularReflectionCubemapRenderer(_engine)
    );
    
    buildBRDF();
    
    vulkan::factory::Sampler samplerFactory(_engine);
    _cubeMapSampler = samplerFactory.build();
    _irradianceMapSampler = samplerFactory.build();
    _specularReflectionSampler = samplerFactory.build();
    _brdfIntegrationMapSampler = samplerFactory.build();
    
    _engine->cleanupManager().push([&](VkDevice device) {
        _brdfIntegrationMap.reset();
        vkDestroySampler(device, _cubeMapSampler, nullptr);
        vkDestroySampler(device, _irradianceMapSampler, nullptr);
        vkDestroySampler(device, _specularReflectionSampler, nullptr);
        vkDestroySampler(device, _brdfIntegrationMapSampler, nullptr);
    });
}

EnvironmentResources::EnvironmentResources(
    bg2e::render::Engine * engine,
    const std::vector<VkFormat>& targetImages,
    VkFormat depthFormat,
    VkSampleCountFlagBits sampleCount
)
    :_engine(engine)
    ,_depthImageFormat(depthFormat)
{
    _targetImagesFormat.assign(targetImages.begin(), targetImages.end());
    
    _sphereToCubemap = std::unique_ptr<SphereToCubemapRenderer>(
        new SphereToCubemapRenderer(_engine)
    );
    _irradianceRenderer = std::unique_ptr<IrradianceCubemapRenderer>(
        new IrradianceCubemapRenderer(_engine)
    );
    _specularRenderer = std::unique_ptr<SpecularReflectionCubemapRenderer>(
        new SpecularReflectionCubemapRenderer(_engine)
    );
    _skyboxRenderer = std::unique_ptr<SkyboxRenderer>(
        new SkyboxRenderer(_engine)
    );
    _skyboxRenderer->setSampleCount(sampleCount);
    
    buildBRDF();
    
    vulkan::factory::Sampler samplerFactory(_engine);
    _cubeMapSampler = samplerFactory.build();
    _irradianceMapSampler = samplerFactory.build();
    
    // TODO: get this value from the specular renderer
    samplerFactory.createInfo.maxLod = 10;
    _specularReflectionSampler = samplerFactory.build();
    samplerFactory.createInfo.maxLod = 0;
    _brdfIntegrationMapSampler = samplerFactory.build();
    
    _engine->cleanupManager().push([&](VkDevice device) {
        _brdfIntegrationMap.reset();
        vkDestroySampler(device, _cubeMapSampler, nullptr);
        vkDestroySampler(device, _irradianceMapSampler, nullptr);
        vkDestroySampler(device, _specularReflectionSampler, nullptr);
        vkDestroySampler(device, _brdfIntegrationMapSampler, nullptr);
    });
}

void EnvironmentResources::initFrameResources(bg2e::render::vulkan::DescriptorSetAllocator *frameAllocator)
{
    _sphereToCubemap->initFrameResources(frameAllocator);
    _irradianceRenderer->initFrameResources(frameAllocator);
    _specularRenderer->initFrameResources(frameAllocator);
    
    if (_skyboxRenderer.get())
    {
        _skyboxRenderer->initFrameResources(frameAllocator);
    }
}

void EnvironmentResources::build(
    const std::filesystem::path& environmentTexture,
    VkExtent2D cubeMapSize,
    VkExtent2D irradianceMapSize,
    VkExtent2D specularReflectionSize
) {
    _sphereToCubemap->build(environmentTexture, cubeMapSize);
    _cubemapChanged = true;
    _irradianceRenderer->build(_sphereToCubemap->cubeMapImage(), irradianceMapSize);
    _specularRenderer->build(_sphereToCubemap->cubeMapImage(), specularReflectionSize);
    
    if (_skyboxRenderer.get())
    {
        auto bt = new base::Texture();
        auto skyboxSource = _specularRenderer->cubeMapImage();
        if (_skyboxBlurLevel > 0)
        {
            bt->setMaxLod(static_cast<float>(_skyboxBlurLevel));
            bt->setMinLod(static_cast<float>(_skyboxBlurLevel));
            bt->setMipLodBias(0.0f);
            bt->setMagFilter(base::Texture::FilterLinear);
            bt->setMinFilter(base::Texture::FilterLinear);
        }
        
        _cubeMapTexture = std::shared_ptr<bg2e::render::Texture>(
            new bg2e::render::Texture(_engine, bt, skyboxSource)
        );

        _engine->cleanupManager().push([&](VkDevice) {
            _cubeMapTexture.reset();
            _skyboxRenderer.reset();
            _sphereToCubemap.reset();
        });
        _skyboxRenderer->build(
            _cubeMapTexture,
            _targetImagesFormat,
            _depthImageFormat
        );
    }
    else
    {
        _engine->cleanupManager().push([&](VkDevice) {
            _sphereToCubemap = nullptr;
        });
    }
}

void EnvironmentResources::build(
    std::shared_ptr<render::Texture> texture,
    VkExtent2D cubeMapSize,
    VkExtent2D irradianceMapSize,
    VkExtent2D specularReflectionSize
) {
    _sphereToCubemap->build(texture, cubeMapSize);
    _cubemapChanged = true;
    _irradianceRenderer->build(_sphereToCubemap->cubeMapImage(), irradianceMapSize);
    _specularRenderer->build(_sphereToCubemap->cubeMapImage(), specularReflectionSize);
    
    if (_skyboxRenderer.get())
    {
        auto bt = new base::Texture();
        auto skyboxSource = _specularRenderer->cubeMapImage();
        if (_skyboxBlurLevel > 0)
        {
            bt->setMaxLod(static_cast<float>(_skyboxBlurLevel));
            bt->setMinLod(static_cast<float>(_skyboxBlurLevel));
            bt->setMipLodBias(0.0f);
            bt->setMagFilter(base::Texture::FilterLinear);
            bt->setMinFilter(base::Texture::FilterLinear);
        }
        
        _cubeMapTexture = std::shared_ptr<bg2e::render::Texture>(
            new bg2e::render::Texture(_engine, bt, skyboxSource)
        );
        
        _engine->cleanupManager().push([&](VkDevice) {
            _cubeMapTexture.reset();
            _skyboxRenderer.reset();
            _sphereToCubemap.reset();
        });
        _skyboxRenderer->build(
            _cubeMapTexture,
            _targetImagesFormat,
            _depthImageFormat
        );
    }
    else
    {
        _engine->cleanupManager().push([&](VkDevice) {
            _sphereToCubemap = nullptr;
        });
    }
}

void EnvironmentResources::swapEnvironmentTexture(const std::filesystem::path& environmentTexture)
{
    _sphereToCubemap->updateImage(environmentTexture);
    _cubemapChanged = true;
}

void EnvironmentResources::swapEnvironmentTexture(std::shared_ptr<render::Texture> texture)
{
    _sphereToCubemap->updateImage(texture);
    _cubemapChanged = true;
}

void EnvironmentResources::setSkyboxBlurLevel(int blurLevel)
{
    _skyboxBlurLevel = blurLevel;
    _blurChanged = true;
}

void EnvironmentResources::update(
    VkCommandBuffer cmd,
    uint32_t currentFrame,
    bg2e::render::vulkan::FrameResources& frameResources
) {
    if (_cubemapChanged)
    {
        _sphereToCubemap->update(cmd, frameResources);
        _irradianceRenderer->update(cmd, currentFrame, frameResources);
        _specularRenderer->update(cmd, currentFrame, frameResources);
        _cubemapChanged = false;
    }
}

void EnvironmentResources::updateSkybox(
    const glm::mat4& viewMatrix,
    const glm::mat4& projMatrix
) {
    if (_skyboxRenderer.get())
    {
        if (_blurChanged)
        {
            auto bt = new base::Texture();
            if (_skyboxBlurLevel > 0)
            {
                bt->setMaxLod(static_cast<float>(_skyboxBlurLevel));
                bt->setMinLod(static_cast<float>(_skyboxBlurLevel));
                bt->setMipLodBias(0.0f);
                bt->setMagFilter(base::Texture::FilterLinear);
                bt->setMinFilter(base::Texture::FilterLinear);
            }
            _cubeMapTexture->updateSampler(bt);
            _blurChanged = false;
        }
        _skyboxRenderer->update(viewMatrix, projMatrix);
    }
}

void EnvironmentResources::drawSkybox(
    VkCommandBuffer cmd,
    uint32_t currentFrame,
    vulkan::FrameResources& frameResources
) {
    if (_skyboxRenderer.get())
    {
        _skyboxRenderer->draw(cmd, currentFrame, frameResources);
    }
}

void EnvironmentResources::buildBRDF()
{
    auto brdfIntegrationMapGenerator = new render::BRDFIntegrationMapTextureGenerator(_engine, 512, 512);
    _brdfIntegrationMap = std::shared_ptr<Texture>(brdfIntegrationMapGenerator->generate());
}

}
}
