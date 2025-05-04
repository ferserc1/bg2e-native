
#include <bg2e/render/EnvironmentResources.hpp>

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
}

void EnvironmentResources::initFrameResources(bg2e::render::vulkan::DescriptorSetAllocator *frameAllocator)
{
    _sphereToCubemap->initFrameResources(frameAllocator);
    _irradianceRenderer->initFrameResources(frameAllocator);
    _specularRenderer->initFrameResources(frameAllocator);
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

}
}
