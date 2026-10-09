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

#pragma once

#include <bg2e/common.hpp>
#include <bg2e/gpu/Common.hpp>
#include <bg2e/gpu/detail/SubmissionState.hpp>
#include <bg2e/gpu/Buffer.hpp>
#include <bg2e/gpu/CubeMap.hpp>
#include <bg2e/gpu/GraphicsPipeline.hpp>
#include <bg2e/gpu/ComputePipeline.hpp>
#include <bg2e/gpu/RayTracingMesh.hpp>
#include <bg2e/gpu/RayTracingPipeline.hpp>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>

namespace bg2e {
namespace gpu {

class CleanupManager;
namespace vk { class Queue; }
namespace metal { class Queue; }
class Buffer;
class CommandBuffer;
class CubeMap;
class Image;
class Instance;
class PhysicalDevice;
class ResourceSet;
class Surface;
class Queue;
class Sampler;
class ShaderModule;
class PipelineLayout;
class GraphicsPipeline;
class ComputePipeline;
class RayTracingMesh;
class RayTracingScene;
class RayTracingPipeline;

class BG2E_API Device {
public:
    virtual ~Device() = default;


    virtual void create(Instance* instance, PhysicalDevice* physicalDevice, Surface* surface) = 0;
    virtual void cleanup() = 0;
    // Drains wrapper-managed sends while temporarily blocking new submissions.
    // Admission reopens on return; callers coordinate subsequent resource edits.
    // Direct sends through native handles are outside this contract.
    virtual void waitIdle() = 0;

    virtual bool isValid() const = 0;

    virtual const Queue& graphicsQueue() const = 0;
    virtual const Queue& presentQueue() const = 0;
    virtual const Queue& transferQueue() const = 0;

    virtual std::shared_ptr<Sampler> createSampler(const SamplerDescription& description)
    {
        throw std::runtime_error("createSampler not implemented");
    }

    virtual std::shared_ptr<ResourceSet> createResourceSet(PipelineLayout* layout, uint32_t setIndex, const std::string& debugName = {})
    {
        throw std::runtime_error("createResourceSet not implemented");
    }

    virtual std::shared_ptr<Image> createImage(const ImageDescription& description)
    {
        throw std::runtime_error("createImage not implemented");
    }

    virtual std::shared_ptr<CubeMap> createCubeMap(const CubeMapDescription& description)
    {
        throw std::runtime_error("createCubeMap not implemented");
    }

    virtual std::shared_ptr<ShaderModule> createShaderModule(const ShaderModuleDescription& description)
    {
        throw std::runtime_error("createShaderModule not implemented");
    }

    virtual std::shared_ptr<PipelineLayout> createPipelineLayout(const PipelineLayoutDescription& description)
    {
        throw std::runtime_error("createPipelineLayout not implemented");
    }

    virtual std::shared_ptr<GraphicsPipeline> createGraphicsPipeline(const GraphicsPipelineDescription& description)
    {
        throw std::runtime_error("createGraphicsPipeline not implemented");
    }

    virtual std::shared_ptr<ComputePipeline> createComputePipeline(const ComputePipelineDescription& description)
    {
        throw std::runtime_error("createComputePipeline not implemented");
    }

    virtual std::shared_ptr<Buffer> createBuffer(const std::string& debugName = {})
    {
        throw std::runtime_error("createBuffer not implemented");
    }

    virtual void immediateSubmit(std::function<void(CommandBuffer* cmd)>&& function)
    {
        throw std::runtime_error("immediateSubmit not implemented");
    }

    // --- Ray tracing acceleration structures ---------------------------------
    // RayTracingMesh is created from existing GPU vertex/index buffers (see
    // RayTracingMeshDescription). RayTracingScene owns the scene-level
    // acceleration structure and the reusable instance/scratch buffers.

    virtual std::shared_ptr<RayTracingMesh> createRayTracingMesh(const RayTracingMeshDescription& description)
    {
        throw std::runtime_error("createRayTracingMesh not implemented");
    }

    virtual std::shared_ptr<RayTracingScene> createRayTracingScene(const std::string& debugName = {})
    {
        throw std::runtime_error("createRayTracingScene not implemented");
    }

    virtual std::shared_ptr<RayTracingPipeline> createRayTracingPipeline(
        const RayTracingPipelineDescription& description)
    {
        throw std::runtime_error("createRayTracingPipeline not implemented");
    }
protected:
    // Internal coordination, deliberately absent from the public Device API.
    const std::shared_ptr<detail::SubmissionState>& submissionState() const { return _submissions; }
    friend class CleanupManager;
    friend class vk::Queue;
    friend class metal::Queue;
    std::shared_ptr<detail::SubmissionState> _submissions = std::make_shared<detail::SubmissionState>();
};

}
}
