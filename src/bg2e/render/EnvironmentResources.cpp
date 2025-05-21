
#include <bg2e/render/EnvironmentResources.hpp>
#include <bg2e/render/vulkan/factory/Sampler.hpp>

namespace bg2e {
namespace render {

EnvironmentResources::EnvironmentResources(bg2e::render::Vulkan *vulkan)
    :_vulkan(vulkan)
{
    _sphereToCubemap = std::unique_ptr<SphereToCubemapRenderer>(
        new SphereToCubemapRenderer(_vulkan)
    );
    _irradianceRenderer = std::unique_ptr<IrradianceCubemapRenderer>(
        new IrradianceCubemapRenderer(_vulkan)
    );
    _specularRenderer = std::unique_ptr<SpecularReflectionCubemapRenderer>(
        new SpecularReflectionCubemapRenderer(_vulkan)
    );
    
    vulkan::factory::Sampler samplerFactory(_vulkan);
    _cubeMapSampler = samplerFactory.build();
    _irradianceMapSampler = samplerFactory.build();
    _specularReflectionSampler = samplerFactory.build();
    
    _vulkan->cleanupManager().push([&](VkDevice device) {
        vkDestroySampler(device, _cubeMapSampler, nullptr);
        vkDestroySampler(device, _irradianceMapSampler, nullptr);
        vkDestroySampler(device, _specularReflectionSampler, nullptr);
    });
}

EnvironmentResources::EnvironmentResources(bg2e::render::Vulkan* vulkan, const std::vector<VkFormat>& targetImages, VkFormat depthFormat)
    :_vulkan(vulkan)
    ,_depthImageFormat(depthFormat)
{
    _targetImagesFormat.assign(targetImages.begin(), targetImages.end());
    
    _sphereToCubemap = std::unique_ptr<SphereToCubemapRenderer>(
        new SphereToCubemapRenderer(_vulkan)
    );
    _irradianceRenderer = std::unique_ptr<IrradianceCubemapRenderer>(
        new IrradianceCubemapRenderer(_vulkan)
    );
    _specularRenderer = std::unique_ptr<SpecularReflectionCubemapRenderer>(
        new SpecularReflectionCubemapRenderer(_vulkan)
    );
    _skyboxRenderer = std::unique_ptr<SkyboxRenderer>(
        new SkyboxRenderer(_vulkan)
    );
    
    vulkan::factory::Sampler samplerFactory(_vulkan);
    _cubeMapSampler = samplerFactory.build();
    _irradianceMapSampler = samplerFactory.build();
    _specularReflectionSampler = samplerFactory.build();
    
    _vulkan->cleanupManager().push([&](VkDevice device) {
        vkDestroySampler(device, _cubeMapSampler, nullptr);
        vkDestroySampler(device, _irradianceMapSampler, nullptr);
        vkDestroySampler(device, _specularReflectionSampler, nullptr);
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
        _cubeMapTexture = std::shared_ptr<bg2e::render::Texture>(
            new bg2e::render::Texture(_vulkan, _sphereToCubemap->cubeMapImage())
        );
        _vulkan->cleanupManager().push([&](VkDevice) {
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
        _vulkan->cleanupManager().push([&](VkDevice) {
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
        _cubeMapTexture = std::shared_ptr<bg2e::render::Texture>(
            new bg2e::render::Texture(_vulkan, _sphereToCubemap->cubeMapImage())
        );
        _vulkan->cleanupManager().push([&](VkDevice) {
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
        _vulkan->cleanupManager().push([&](VkDevice) {
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

}
}
