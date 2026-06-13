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

#include <bg2e/gpu/metal/ComputePipeline.hpp>
#include <bg2e/gpu/metal/ShaderModule.hpp>
#include <bg2e/gpu/metal/PipelineLayout.hpp>
#include <bg2e/base/Log.hpp>

#include <stdexcept>

namespace bg2e {
namespace gpu {
namespace metal {

#if BG2E_IS_MAC

ComputePipeline::ComputePipeline(gpu::Device* gpuDevice, MTL::Device* device, const gpu::ComputePipelineDescription& description)
    : gpu::ComputePipeline(gpuDevice)
{
    auto* metalComputeModule = dynamic_cast<metal::ShaderModule*>(description.computeShader);
    auto* metalLayout = dynamic_cast<metal::PipelineLayout*>(description.layout);

    if (!metalComputeModule)
    {
        throw std::runtime_error("metal::ComputePipeline: description contains non-Metal shader module");
    }

    _layout = metalLayout;

    if (!metalComputeModule->isValid())
    {
        throw std::runtime_error("metal::ComputePipeline: compute shader module is not valid");
    }

    NS::Error* error = nullptr;
    _computePipelineState = device->newComputePipelineState(metalComputeModule->function(), &error);

    if (!_computePipelineState)
    {
        std::string errorMsg = "metal::ComputePipeline: failed to create compute pipeline state";
        if (error)
        {
            errorMsg += " - " + std::string(error->localizedDescription()->utf8String());
        }
        throw std::runtime_error(errorMsg);
    }

    _maxTotalThreadsPerThreadgroup = _computePipelineState->maxTotalThreadsPerThreadgroup();
    _threadExecutionWidth = _computePipelineState->threadExecutionWidth();
}

ComputePipeline::~ComputePipeline()
{
    cleanup();
}

bool ComputePipeline::isValid() const
{
    return _computePipelineState != nullptr;
}

void ComputePipeline::cleanup()
{
    if (_computePipelineState)
    {
        _computePipelineState->release();
        _computePipelineState = nullptr;
    }
    _maxTotalThreadsPerThreadgroup = 0;
    _threadExecutionWidth = 0;
}

#else

ComputePipeline::ComputePipeline(gpu::Device* gpuDevice, void* /*device*/, const gpu::ComputePipelineDescription& /*description*/)
    : gpu::ComputePipeline(gpuDevice)
{
}

ComputePipeline::~ComputePipeline()
{
    cleanup();
}

bool ComputePipeline::isValid() const
{
    return false;
}

void ComputePipeline::cleanup()
{
}

#endif

}
}
}
