//
//  BRDFIntegrationMapTextureGenerator.cpp
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
    
    GPUProcess gpu(_engine);
    gpu.addBinding(0, image, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
    gpu.executeShader("brdf_lut.comp.spv", image->extent2D());
    
    return wrapImage(
        image,
        false,
        base::Texture::FilterLinear,
        base::Texture::FilterLinear
    );
}
        
}
