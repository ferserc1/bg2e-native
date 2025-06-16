
#include <bg2e/scene/vk/LightDataBinding.hpp>
#include <bg2e/render/vulkan/macros/frame_resources.hpp>
#include <bg2e/render/vulkan/factory/DescriptorSetLayout.hpp>

namespace bg2e::scene::vk {

void LightDataBinding::initFrameResources(bg2e::render::vulkan::DescriptorSetAllocator * frameAllocator)
{
    frameAllocator->requirePoolSizeRatio(1, {
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, BG2E_MAX_FORWARD_LIGHTS }
    });
}

VkDescriptorSetLayout LightDataBinding::createLayout()
{
    if (_layout == VK_NULL_HANDLE)
    {
        bg2e::render::vulkan::factory::DescriptorSetLayout dsFactory;

        dsFactory.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

        _layout = dsFactory.build(_engine->device().handle(), VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT);
    }
    return _layout;
}

VkDescriptorSet LightDataBinding::newDescriptorSet(
    bg2e::render::vulkan::FrameResources & frameResources,
    const LightUniforms & lights
) {
    if (_layout == VK_NULL_HANDLE)
    {
        throw std::runtime_error("LightDataBinding::newDescriptorSet() - The descriptor set layout is not created");
    }

    auto uniformBuffer = bg2e::render::vulkan::macros::createBuffer(_engine, frameResources, lights);
    auto ds = frameResources.newDescriptorSet(_layout);
    ds->beginUpdate();
        ds->addBuffer(
            0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            uniformBuffer, sizeof(LightUniforms), 0
        );
    ds->endUpdate();
    return ds->descriptorSet();
}

}
