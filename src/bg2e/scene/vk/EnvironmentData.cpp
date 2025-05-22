
#include <bg2e/scene/vk/EnvironmentData.hpp>
#include <bg2e/render/vulkan/macros/frame_resources.hpp>
#include <bg2e/render/vulkan/factory/DescriptorSetLayout.hpp>

namespace bg2e::scene::vk {

void EnvironmentData::initFrameResources(bg2e::render::vulkan::DescriptorSetAllocator * frameAllocator)
{
    frameAllocator->requirePoolSizeRatio(1, {
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2 }
    });
}

VkDescriptorSetLayout EnvironmentData::createLayout()
{
    if (_layout == VK_NULL_HANDLE)
    {
        bg2e::render::vulkan::factory::DescriptorSetLayout dsFactory;
        
        dsFactory.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        // TODO: Add specular reflection map
        
        _layout = dsFactory.build(_vulkan->device().handle(), VK_SHADER_STAGE_FRAGMENT_BIT);
    }
    return _layout;
}

VkDescriptorSet EnvironmentData::newDescriptorSet(
    bg2e::render::vulkan::FrameResources & frameResources,
    bg2e::render::EnvironmentResources * environmentResources
) {
    if (_layout == VK_NULL_HANDLE)
    {
        throw std::runtime_error("EnvironmentData::newDescriptorSet() - The descriptor set layout is not created");
    }
    
    auto ds = frameResources.newDescriptorSet(_layout);
    ds->beginUpdate();
        ds->addImage(
            0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            environmentResources->irradianceMapImage()->imageView(),
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            environmentResources->irradianceMapSampler()
        );
        
        // TODO: Add specular reflection map
    ds->endUpdate();
    return ds->descriptorSet();
}


}
