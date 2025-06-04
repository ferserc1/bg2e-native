
#include <bg2e/common.hpp>
#include <bg2e/render/vulkan/factory/Sampler.hpp>

namespace bg2e::render::vulkan::factory {

Sampler::Sampler(Engine * engine)
	:_engine(engine)
{
	createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	createInfo.anisotropyEnable = VK_TRUE;
	createInfo.maxAnisotropy = 16;
	createInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	createInfo.unnormalizedCoordinates = VK_FALSE;
	createInfo.compareEnable = VK_FALSE;
	createInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	createInfo.mipLodBias = 0.0f;
	createInfo.minLod = 0.0f;
	createInfo.maxLod = 0.0f;
}

VkSamplerCreateInfo createInfo;

VkSampler Sampler::build(
	VkFilter magFilter,
	VkFilter minFilter,
	VkSamplerAddressMode addressModeU,
	VkSamplerAddressMode addressModeV,
	VkSamplerAddressMode addressModeW
) {
	createInfo.magFilter = magFilter;
	createInfo.minFilter = minFilter;
	createInfo.addressModeU = addressModeU;
	createInfo.addressModeV = addressModeV;
	createInfo.addressModeW = addressModeW;

	VkSampler sampler;
	vkCreateSampler(_engine->device().handle(), &createInfo, nullptr, &sampler);

	return sampler;
}

}
