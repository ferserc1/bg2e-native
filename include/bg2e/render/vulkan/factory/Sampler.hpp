#pragma once

#include <bg2e/common.hpp>
#include <bg2e/render/vulkan/common.hpp>
#include <bg2e/render/Vulkan.hpp>


namespace bg2e {
namespace render {
namespace vulkan {
namespace factory {

class BG2E_API Sampler {
public:
    Sampler(Vulkan * vulkan);
    
    VkSamplerCreateInfo createInfo;

    VkSampler build(
        VkFilter magFilter = VK_FILTER_LINEAR,
        VkFilter minFilter = VK_FILTER_LINEAR,
        VkSamplerAddressMode addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        VkSamplerAddressMode addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        VkSamplerAddressMode addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT
    );
    
protected:
    Vulkan * _vulkan;
};

}
}
}
}
