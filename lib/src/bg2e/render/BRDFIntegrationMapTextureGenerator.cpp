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

#include <bg2e/render/BRDFIntegrationMapTextureGenerator.hpp>
#include <bg2e/math/all.hpp>
#include <bg2e/base/PlatformTools.hpp>
#include <bg2e/render/vulkan/factory/ComputePipeline.hpp>
#include <bg2e/render/vulkan/factory/DescriptorSetLayout.hpp>
#include <bg2e/render/vulkan/factory/PipelineLayout.hpp>
#include <bg2e/render/GPUProcess.hpp>

namespace bg2e::render {

Texture* BRDFIntegrationMapTextureGenerator::generate()
{
    auto image = createImage(
        VK_FORMAT_R16G16B16A16_SFLOAT,
        { _width, _height },
        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT
    );
    
    vulkan::Image::transitionImage(
        _engine,
        image->handle(),
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_GENERAL
    );

    GPUProcess gpu(_engine);
    gpu.addBinding(0, image, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_IMAGE_LAYOUT_GENERAL);
    gpu.executeShader("brdf_lut.comp.spv", image->extent2D());
    
    return wrapImage(
        image,
        false,
        base::Texture::FilterLinear,
        base::Texture::FilterLinear
    );
}
        
}
