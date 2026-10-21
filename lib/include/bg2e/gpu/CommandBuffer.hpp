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
#include <bg2e/gpu/detail/SubmissionTimingState.hpp>
#include <bg2e/gpu/detail/SubmissionState.hpp>
#include <stdexcept>
#include <cstring>
#include <exception>

namespace bg2e {
namespace gpu {

namespace vk { class Queue; }
namespace metal { class Queue; }

class Device;
class Buffer;
class CubeMap;
class Image;
class ResourceSet;
class SurfaceFrame;
class GraphicsPipeline;
class ComputePipeline;
class RayTracingMesh;
class RayTracingScene;
class RayTracingPipeline;

class BG2E_API CommandBuffer {
  public:
    virtual ~CommandBuffer()
    {
        if (_submissionTiming)
        {
            _submissionTiming->abandon();
        }
        if (_recordingCompletion)
        {
            _recordingCompletion->abandon();
        }
    }

    // Stable identity for this recording, independent of wrapper reset/destruction.
    // Completion is false until successful submission and normal GPU completion.
    std::shared_ptr<const detail::RecordingCompletion> recordingCompletion() const
    {
        return _recordingCompletion;
    }

    virtual void begin() = 0;
    virtual void end() = 0;

    // Request immediately after begin(), before workload commands. Defaults are
    // nonthrowing unsupported; identities are supplied by the caller.
    virtual SubmissionTimingStatus requestSubmissionTiming(const SubmissionTimingRequest&) noexcept
    {
        return SubmissionTimingStatus::Unsupported;
    }
    // Recording-thread only. Zero means disabled/omitted; IDs are local to this
    // command recording. Labels are copied and contexts preserve layer identity.
    uint32_t beginTimingInterval(const char* label, uint64_t context = 0, bool detailedOnly = false) noexcept
    {
        if (!_submissionTiming || _submissionTiming->terminal || _submissionTiming->submitted || !_submissionTiming->sample.passTiming ||
            (detailedOnly && !_submissionTiming->sample.detailed))
        {
            return 0;
        }
        auto& sample = _submissionTiming->sample;
        if (sample.intervalCount == sample.maximumIntervals || (detailedOnly && sample.intervalCount >= sample.maximumIntervals - 8))
        {
            ++sample.intervalsDropped;
            return 0;
        }
        auto& interval = sample.intervals[sample.intervalCount];
        interval.id = ++sample.intervalCount;
        interval.context = context;
        if (label)
        {
            std::strncpy(interval.label.data(), label, interval.label.size() - 1);
        }
        for (uint32_t i = interval.id - 1; i > 0; --i)
        {
            if (!sample.intervals[i - 1].ended)
            {
                interval.parent = i;
                break;
            }
        }
        timingIntervalMarker(interval.id - 1, false);
        return interval.id;
    }

    void endTimingInterval(uint32_t id, bool aborted = false) noexcept
    {
        if (!_submissionTiming || _submissionTiming->terminal || _submissionTiming->submitted || !id || id > _submissionTiming->sample.intervalCount)
        {
            return;
        }
        auto& interval = _submissionTiming->sample.intervals[id - 1];
        if (!interval.ended)
        {
            timingIntervalMarker(id - 1, true);
            interval.ended = true;
            if (aborted)
            {
                interval.status = SubmissionTimingStatus::Aborted;
            }
        }
    }

    void setSubmissionTimingContext(uint64_t scene, SubmissionTimingWorkload workload) noexcept
    {
        if (_submissionTiming)
        {
            _submissionTiming->context(scene, workload);
        }
    }

    virtual void transition(gpu::Image* image, ImageLayout newLayout) = 0;
    virtual void transition(gpu::Image* image, ImageLayout newLayout, const ImageSubresourceRange& range)
    {
        throw std::runtime_error("Subresource transitions are not implemented");
    }
    // Exact copies: matching color formats or depth-only D16/D32; no stencil.
    virtual void copyImage(const ImageView& source, const ImageView& destination)
    {
        throw std::runtime_error("Subresource copies are not implemented");
    }
    virtual void generateMipmaps(gpu::Image* image, ImageLayout finalLayout = ImageLayout::ShaderReadOnly)
    {
        throw std::runtime_error("Mipmap generation is not implemented");
    }
    static void validateCopy(const ImageView& source, const ImageView& destination, const Device& device);
    static void validateMipmapGeneration(const gpu::Image& image, const Device& device, ImageLayout finalLayout);

    // Explicit attachment path. Layout transitions are the caller's responsibility.
    virtual void beginRendering(const RenderingDescription& description)
    {
        throw std::runtime_error("Attachment-array rendering is not implemented");
    }
    static Size2D validateRendering(const RenderingDescription& description, const Device& device);
    static void validatePipelineAttachments(const RenderingDescription& description, const GraphicsPipeline& pipeline);

    virtual void beginRendering(gpu::SurfaceFrame* frame) = 0;
    virtual void beginRendering(gpu::Image* colorImage, uint32_t mipLevel = 0)
    {
        throw std::runtime_error("beginRendering(Image*) not implemented");
    }
    virtual void beginRendering(gpu::Image* colorImage, gpu::Image* depthImage, uint32_t mipLevel = 0)
    {
        throw std::runtime_error("beginRendering(Image*, Image*) not implemented");
    }
    virtual void beginRendering(gpu::CubeMap* cubemap, CubemapFace face, uint32_t mipLevel = 0)
    {
        throw std::runtime_error("beginRendering(CubeMap*) not implemented");
    }
    virtual void endRendering() = 0;

    virtual void beginCompute() = 0;
    virtual void endCompute() = 0;

    virtual void clearColor(uint32_t attachmentIndex, const gpu::Color& color) = 0;
    virtual void clearDepth(float depth) = 0;

    virtual void bindPipeline(gpu::GraphicsPipeline* pipeline)
    {
        throw std::runtime_error("bindPipeline not implemented");
    }

    virtual void draw(uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t firstVertex = 0, uint32_t firstInstance = 0)
    {
        throw std::runtime_error("draw not implemented");
    }

    virtual void bindPipeline(gpu::ComputePipeline* pipeline)
    {
        throw std::runtime_error("bindPipeline(ComputePipeline) not implemented");
    }

    virtual void dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
    {
        throw std::runtime_error("dispatch not implemented");
    }

    virtual void pushConstants(ShaderStage stage, uint32_t offset, uint32_t size, const void* data)
    {
        throw std::runtime_error("pushConstants not implemented");
    }

    virtual void bindResourceSet(gpu::GraphicsPipeline* pipeline, uint32_t setIndex, gpu::ResourceSet* set)
    {
        throw std::runtime_error("bindResourceSet(GraphicsPipeline) not implemented");
    }

    virtual void bindResourceSet(gpu::ComputePipeline* pipeline, uint32_t setIndex, gpu::ResourceSet* set)
    {
        throw std::runtime_error("bindResourceSet(ComputePipeline) not implemented");
    }

    virtual void bindVertexBuffer(uint32_t binding, gpu::Buffer* buffer, uint64_t offset = 0)
    {
        throw std::runtime_error("bindVertexBuffer not implemented");
    }

    virtual void bindIndexBuffer(gpu::Buffer* buffer, uint64_t offset = 0)
    {
        throw std::runtime_error("bindIndexBuffer not implemented");
    }

    virtual void drawIndexed(uint32_t indexCount, uint32_t instanceCount = 1,
                             uint32_t firstIndex = 0, int32_t vertexOffset = 0,
                             uint32_t firstInstance = 0)
    {
        throw std::runtime_error("drawIndexed not implemented");
    }

    virtual void copyImage(gpu::Image* src, gpu::Image* dst)
    {
        throw std::runtime_error("copyImage not implemented");
    }

    // TODO: function not fully tested
    virtual void blitImage(gpu::Image* src, gpu::Image* dst)
    {
        throw std::runtime_error("blitImage not implemented");
    }

    // --- Ray tracing acceleration structure builds ---------------------------
    // Both Vulkan and Metal build acceleration structures through GPU commands,
    // so these are recorded into a command buffer. They must be called outside
    // an active rendering scope.

    virtual void buildRayTracingMesh(gpu::RayTracingMesh* mesh)
    {
        throw std::runtime_error("buildRayTracingMesh not implemented");
    }

    virtual void buildRayTracingScene(gpu::RayTracingScene* scene)
    {
        throw std::runtime_error("buildRayTracingScene not implemented");
    }

    virtual void bindPipeline(gpu::RayTracingPipeline* pipeline)
    {
        throw std::runtime_error("bindPipeline(RayTracingPipeline) not implemented");
    }

    virtual void bindResourceSet(gpu::RayTracingPipeline* pipeline, uint32_t setIndex, gpu::ResourceSet* set)
    {
        throw std::runtime_error("bindResourceSet(RayTracingPipeline) not implemented");
    }

    virtual void traceRays(uint32_t width, uint32_t height, uint32_t depth = 1)
    {
        throw std::runtime_error("traceRays not implemented");
    }

    // Used by coordinators to enforce callback scope ownership.
    virtual bool hasActiveScope() const = 0;
    virtual bool isValid() const = 0;

  protected:
    virtual void timingIntervalMarker(uint32_t index, bool) noexcept
    {
        _submissionTiming->sample.intervals[index].status = SubmissionTimingStatus::Unsupported;
    }

    std::shared_ptr<detail::SubmissionTimingState> _timingState;
    std::shared_ptr<detail::SubmissionTimingTicket> _submissionTiming;
    // Backend begin() calls this only after native reset/begin succeeds.
    void beginRecordingCompletion()
    {
        if (_submissionTiming)
        {
            _submissionTiming->abandon();
            _submissionTiming.reset();
        }
        auto next = std::make_shared<detail::RecordingCompletion>();
        if (_recordingCompletion)
        {
            _recordingCompletion->abandon();
        }
        _recordingCompletion = std::move(next);
    }

  private:
    friend class vk::Queue;
    friend class metal::Queue;

    void publishRecordingSubmission(const std::shared_ptr<detail::CompletionRecord>& record)
    {
        if (_recordingCompletion)
        {
            _recordingCompletion->submitted(record);
        }
    }

    std::shared_ptr<detail::RecordingCompletion> _recordingCompletion;
};

// Pairs endpoints on normal/exceptional exits without changing native scopes.
class GpuTimingScope {
  public:
    GpuTimingScope(CommandBuffer& command, const char* label, uint64_t context = 0, bool detailedOnly = false) noexcept
        : _command(command), _exceptions(std::uncaught_exceptions()), _id(command.beginTimingInterval(label, context, detailedOnly))
    {
    }
    ~GpuTimingScope()
    {
        _command.endTimingInterval(_id, std::uncaught_exceptions() > _exceptions);
    }
    GpuTimingScope(const GpuTimingScope&) = delete;
    GpuTimingScope& operator=(const GpuTimingScope&) = delete;

  private:
    CommandBuffer& _command;
    int _exceptions;
    uint32_t _id;
};

}
}