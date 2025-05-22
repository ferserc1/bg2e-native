#pragma once

#include <bg2e/scene/vk/SceneDataBinding.hpp>
#include <bg2e/render/vulkan/FrameResources.hpp>
#include <bg2e/render/EnvironmentResources.hpp>

namespace bg2e {
namespace scene {
namespace vk {

class BG2E_API EnvironmentDataBinding : public SceneDataBinding {
public:
    EnvironmentDataBinding(bg2e::render::Vulkan * vk) : SceneDataBinding(vk) {}

    void initFrameResources(bg2e::render::vulkan::DescriptorSetAllocator * frameAllocator) override;

    VkDescriptorSetLayout createLayout() override;

    VkDescriptorSet newDescriptorSet(
        bg2e::render::vulkan::FrameResources & frameResources,
        bg2e::render::EnvironmentResources * environmentResources
    );
};

}
}
}
