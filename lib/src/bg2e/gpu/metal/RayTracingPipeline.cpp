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

#include <bg2e/gpu/metal/RayTracingPipeline.hpp>
#include <bg2e/gpu/metal/ShaderModule.hpp>
#include <bg2e/gpu/metal/PipelineLayout.hpp>
#include <bg2e/base/Log.hpp>

#include <stdexcept>

namespace bg2e {
namespace gpu {
namespace metal {

#if BG2E_IS_MAC

RayTracingPipeline::RayTracingPipeline(
    gpu::Device* gpuDevice, MTL::Device* device,
    const gpu::RayTracingPipelineDescription& description)
    : gpu::RayTracingPipeline(gpuDevice)
{
    auto* metalRgenModule = dynamic_cast<metal::ShaderModule*>(description.raygenShader);
    auto* metalLayout = dynamic_cast<metal::PipelineLayout*>(description.layout);

    if (!metalRgenModule)
    {
        throw std::runtime_error("metal::RayTracingPipeline: raygen shader is not a metal::ShaderModule");
    }

    _layout = metalLayout;

    // missShader and closestHitShader are intentionally ignored on Metal.
    // Metal handles miss/hit behavior internally in the compute kernel.

    if (!metalRgenModule->isValid())
    {
        throw std::runtime_error("metal::RayTracingPipeline: raygen shader module is not valid");
    }

    // Create compute pipeline state from the rgen function
    NS::Error* error = nullptr;
    _computePipelineState = device->newComputePipelineState(
        metalRgenModule->function(), &error);

    if (!_computePipelineState)
    {
        std::string errorMsg = "metal::RayTracingPipeline: failed to create compute pipeline state";
        if (error)
        {
            errorMsg += " - " + std::string(error->localizedDescription()->utf8String());
        }
        throw std::runtime_error(errorMsg);
    }

    _maxTotalThreadsPerThreadgroup = _computePipelineState->maxTotalThreadsPerThreadgroup();
}

RayTracingPipeline::~RayTracingPipeline()
{
    cleanup();
}

bool RayTracingPipeline::isValid() const
{
    return _computePipelineState != nullptr;
}

void RayTracingPipeline::cleanup()
{
    if (_computePipelineState)
    {
        _computePipelineState->release();
        _computePipelineState = nullptr;
    }
    _maxTotalThreadsPerThreadgroup = 0;
}

#else

RayTracingPipeline::RayTracingPipeline(
    gpu::Device* gpuDevice, void* /*device*/,
    const gpu::RayTracingPipelineDescription& /*description*/)
    : gpu::RayTracingPipeline(gpuDevice)
{
}

RayTracingPipeline::~RayTracingPipeline()
{
    cleanup();
}

bool RayTracingPipeline::isValid() const
{
    return false;
}

void RayTracingPipeline::cleanup()
{
}

#endif

}
}
}
