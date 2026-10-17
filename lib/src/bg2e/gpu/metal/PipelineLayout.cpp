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

#include <bg2e/gpu/metal/PipelineLayout.hpp>
#include <bg2e/gpu/Device.hpp>

namespace bg2e {
namespace gpu {
namespace metal {

PipelineLayout::PipelineLayout(gpu::Device* gpuDevice, const gpu::PipelineLayoutDescription& description)
    : gpu::PipelineLayout(gpuDevice), _description(description)
{
    validatePushConstantRanges(description.pushConstants);
    validateMetalBufferBindings(description.resourceBindings);
    validateResourceArrays(description, gpuDevice->resourceArrayCapabilities());
    auto domain = [](ShaderStage stage)
    {
        return stage == ShaderStage::Vertex ? 0 : stage == ShaderStage::Fragment ? 1 : 2;
    };
    for (const auto& rb : description.resourceBindings)
    {
        for (auto stage : rb.additionalStages)
        {
            if (domain(stage) != domain(rb.stage))
            {
                throw std::invalid_argument("Metal additional stages must share the same native encoder domain");
            }
        }
        if (rb.count > 1 || rb.metalArgumentBuffer != UINT32_MAX)
        {
            if (rb.metalArgumentBuffer == UINT32_MAX || rb.metalArgumentBuffer >= 31 ||
                (rb.type != ResourceType::SampledImage && rb.type != ResourceType::Sampler) ||
                rb.stage == ShaderStage::Vertex || rb.stage == ShaderStage::Fragment ||
                uint64_t(rb.binding.metal) + rb.count > 4096)
            {
                throw std::invalid_argument("Sampled arrays require explicit compute/RT argument-buffer indices");
            }
            for (auto stage : rb.additionalStages)
            {
                if (stage == ShaderStage::Vertex || stage == ShaderStage::Fragment)
                {
                    throw std::invalid_argument("Sampled argument arrays currently support compute/RT stages only");
                }
            }
            for (const auto& other : description.resourceBindings)
            {
                if (&rb == &other)
                {
                    continue;
                }
                if (other.metalArgumentBuffer == rb.metalArgumentBuffer)
                {
                    if (other.set != rb.set || other.stage != rb.stage ||
                        (uint64_t(rb.binding.metal) < uint64_t(other.binding.metal) + other.count &&
                         uint64_t(other.binding.metal) < uint64_t(rb.binding.metal) + rb.count))
                    {
                        throw std::invalid_argument("Argument-buffer groups must share set/stage and disjoint id ranges");
                    }
                }
                else if (other.metalArgumentBuffer == UINT32_MAX && other.stage != ShaderStage::Vertex &&
                    other.stage != ShaderStage::Fragment && other.binding.metal == rb.metalArgumentBuffer &&
                    (other.type == ResourceType::UniformBuffer || other.type == ResourceType::StorageBuffer ||
                     other.type == ResourceType::AccelerationStructure))
                {
                    throw std::invalid_argument("Argument-buffer slot collides with a direct buffer binding");
                }
            }
        }
    }
}

PipelineLayout::~PipelineLayout()
{
    cleanup();
}

bool PipelineLayout::isValid() const
{
    return true;
}

void PipelineLayout::cleanup()
{
}

}
}
}
