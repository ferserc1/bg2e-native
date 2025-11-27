#pragma once

#include <bg2e/scene/vk/SceneDataBinding.hpp>
#include <bg2e/base/Light.hpp>
#include <glm/glm.hpp>

#include <vector>

#define BG2E_MAX_FORWARD_LIGHTS 8

namespace bg2e {
namespace scene {
namespace vk {

class BG2E_API LightDataBinding : public SceneDataBinding {
public:
    struct LightData {
        glm::vec3 position;
        float intensity;
        base::Color color;
        glm::vec3 direction;
        base::Light::LightType type;
    };

    struct LightUniforms {
        LightData lights[BG2E_MAX_FORWARD_LIGHTS];
        uint32_t lightCount = 0;
        float padding[3]; // Padding to make the struct size a multiple of 16 bytes
    };

    LightDataBinding(bg2e::render::Engine * engine) : SceneDataBinding(engine) {}

    void initFrameResources(bg2e::render::vulkan::DescriptorSetAllocator * frameAllocator) override;

    VkDescriptorSetLayout createLayout() override;

    VkDescriptorSet newDescriptorSet(
        bg2e::render::vulkan::FrameResources & frameResources,
        const LightUniforms& lights
    );
};

}
}
}
