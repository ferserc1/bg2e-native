
#include <bg2e/scene/vk/EnvironmentDataBinding.hpp>
#include <bg2e/render/vulkan/macros/frame_resources.hpp>
#include <bg2e/render/vulkan/factory/DescriptorSetLayout.hpp>

namespace bg2e::scene::vk {

void EnvironmentDataBinding::initFrameResources(bg2e::render::vulkan::DescriptorSetAllocator * frameAllocator)
{
    frameAllocator->requirePoolSizeRatio(1, {
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 }
    });
}

VkDescriptorSetLayout EnvironmentDataBinding::createLayout()
{
    if (_layout == VK_NULL_HANDLE)
    {
        bg2e::render::vulkan::factory::DescriptorSetLayout dsFactory;
        
        dsFactory.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        dsFactory.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        dsFactory.addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        dsFactory.addBinding(3, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        
        _layout = dsFactory.build(_engine->device().handle(), VK_SHADER_STAGE_FRAGMENT_BIT);
    }
    return _layout;
}

VkDescriptorSet EnvironmentDataBinding::newDescriptorSet(
    bg2e::render::vulkan::FrameResources & frameResources,
    bg2e::render::EnvironmentResources * environmentResources
) {
    if (_layout == VK_NULL_HANDLE)
    {
        throw std::runtime_error("EnvironmentData::newDescriptorSet() - The descriptor set layout is not created");
    }
    
    EnvironmentUniforms envUniforms;
    envUniforms.maxEnvMapLod = environmentResources->specularReflectionMapImage()->mipLevels();
    auto uniformBuffer = bg2e::render::vulkan::macros::createBuffer(_engine, frameResources, envUniforms);

    auto ds = frameResources.newDescriptorSet(_layout);
    ds->beginUpdate();
        ds->addImage(
            0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            environmentResources->irradianceMapImage()->imageView(),
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            environmentResources->irradianceMapSampler()
        );
        
        ds->addImage(
            1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            environmentResources->specularReflectionMapImage()->imageView(),
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            environmentResources->specularReflectionMapSampler()
        );
        
        ds->addImage(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            environmentResources->brdfIntegrationMapImage()->image()->imageView(),
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            environmentResources->brdfIntegrationMapSampler()
        );

        ds->addBuffer(3, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            uniformBuffer, sizeof(EnvironmentUniforms), 0
        );

    ds->endUpdate();
    return ds->descriptorSet();
}


}
