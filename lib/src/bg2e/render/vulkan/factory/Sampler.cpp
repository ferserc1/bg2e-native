/*
 *    business grade graphic engine (bg2 engine)
 *    Copyright (C) 2026  Fernando Serrano Carpena
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

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
