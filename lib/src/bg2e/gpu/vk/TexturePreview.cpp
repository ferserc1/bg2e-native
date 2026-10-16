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

#include <bg2e/gpu/vk/TexturePreview.hpp>
#include <bg2e/gpu/vk/Image.hpp>
#include <bg2e/gpu/vk/Sampler.hpp>
#include <imgui.h>
#include <imgui_impl_vulkan.h>
#include <stdexcept>

namespace bg2e::gpu::vk {

uint64_t registerTexturePreview(gpu::Image& image, gpu::Sampler& sampler)
{
    auto* nativeImage = dynamic_cast<vk::Image*>(&image);
    auto* nativeSampler = dynamic_cast<vk::Sampler*>(&sampler);
    if (!nativeImage || !nativeSampler || image.device() != sampler.device())
    {
        throw std::invalid_argument("Preview image/sampler must belong to one Vulkan device");
    }
    return reinterpret_cast<uint64_t>(ImGui_ImplVulkan_AddTexture(nativeSampler->handle(), nativeImage->imageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
}

void removeTexturePreview(uint64_t identifier)
{
    ImGui_ImplVulkan_RemoveTexture(reinterpret_cast<VkDescriptorSet>(identifier));
}

}
