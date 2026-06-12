/*
 *    business grade graphic engine (bg2e engine)
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

#include <bg2e/gpu/vk/Sampler.hpp>
#include <bg2e/base/Log.hpp>

#include <stdexcept>

namespace bg2e {
namespace gpu {
namespace vk {

static VkFilter toVkFilter(gpu::Filter f)
{
    switch (f)
    {
        case gpu::Filter::Nearest: return VK_FILTER_NEAREST;
        case gpu::Filter::Linear:  return VK_FILTER_LINEAR;
    }
    return VK_FILTER_LINEAR;
}

static VkSamplerMipmapMode toVkMipmapMode(gpu::MipmapFilter f)
{
    switch (f)
    {
        case gpu::MipmapFilter::Nearest: return VK_SAMPLER_MIPMAP_MODE_NEAREST;
        case gpu::MipmapFilter::Linear:  return VK_SAMPLER_MIPMAP_MODE_LINEAR;
    }
    return VK_SAMPLER_MIPMAP_MODE_NEAREST;
}

static VkSamplerAddressMode toVkAddressMode(gpu::AddressMode m)
{
    switch (m)
    {
        case gpu::AddressMode::Repeat:         return VK_SAMPLER_ADDRESS_MODE_REPEAT;
        case gpu::AddressMode::ClampToEdge:    return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        case gpu::AddressMode::MirroredRepeat: return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
    }
    return VK_SAMPLER_ADDRESS_MODE_REPEAT;
}

Sampler::Sampler(VkDevice device, const gpu::SamplerDescription& description)
    : _device(device)
{
    VkSamplerCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    createInfo.magFilter = toVkFilter(description.magFilter);
    createInfo.minFilter = toVkFilter(description.minFilter);
    createInfo.mipmapMode = toVkMipmapMode(description.mipFilter);
    createInfo.addressModeU = toVkAddressMode(description.addressModeU);
    createInfo.addressModeV = toVkAddressMode(description.addressModeV);
    createInfo.addressModeW = toVkAddressMode(description.addressModeW);
    createInfo.anisotropyEnable = VK_FALSE;
    createInfo.maxAnisotropy = 1.0f;
    createInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    createInfo.unnormalizedCoordinates = VK_FALSE;
    createInfo.compareEnable = VK_FALSE;
    createInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    createInfo.minLod = 0.0f;
    createInfo.maxLod = 0.0f;
    createInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    createInfo.unnormalizedCoordinates = VK_FALSE;

    VkResult result = vkCreateSampler(_device, &createInfo, nullptr, &_sampler);
    if (result != VK_SUCCESS)
    {
        bg2e_log_error << "vk::Sampler: vkCreateSampler failed with result " << result << bg2e_log_end;
        throw std::runtime_error("Failed to create Vulkan sampler");
    }
}

Sampler::~Sampler()
{
    cleanup();
}

bool Sampler::isValid() const
{
    return _sampler != VK_NULL_HANDLE;
}

void Sampler::cleanup()
{
    if (_sampler != VK_NULL_HANDLE)
    {
        vkDestroySampler(_device, _sampler, nullptr);
        _sampler = VK_NULL_HANDLE;
    }
}

}
}
}
