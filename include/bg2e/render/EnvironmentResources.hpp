
#pragma once

#include <bg2e/render/SphereToCubemapRenderer.hpp>
#include <bg2e/render/IrradianceCubemapRenderer.hpp>
#include <bg2e/render/SpecularReflectionCubemapRenderer.hpp>

#include <memory>

namespace bg2e {
namespace render {

class BG2E_API EnvironmentResources {
public:
    EnvironmentResources(bg2e::render::Vulkan*);
    
    void initFrameResources(bg2e::render::vulkan::DescriptorSetAllocator *frameAllocator);
    
    void build(
        const std::filesystem::path& environmentTexture,
        VkExtent2D cubeMapSize = { 2048, 2048 },
        VkExtent2D irradianceMapSize = { 32, 32 },
        VkExtent2D specularReflectionSize = { 512, 512 }
    );
    
    void update(
        VkCommandBuffer cmd,
        uint32_t currentFrame,
        bg2e::render::vulkan::FrameResources& frameResources
    );

protected:
    bg2e::render::Vulkan * _vulkan;
    
    std::unique_ptr<SphereToCubemapRenderer> _sphereToCubemap;
    std::unique_ptr<IrradianceCubemapRenderer> _irradianceRenderer;
    std::unique_ptr<SpecularReflectionCubemapRenderer> _specularRenderer;
    bool _cubemapChanged = true;
};

}
}
