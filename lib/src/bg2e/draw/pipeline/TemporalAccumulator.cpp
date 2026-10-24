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

#include <bg2e/draw/pipeline/TemporalAccumulator.hpp>
#include <bg2e/draw/pipeline/TemporaryResourcePool.hpp>
#include <bg2e/base/CpuProfile.hpp>
#include <bg2e/gpu/ShaderLib.hpp>
#include <bg2e/base/PlatformTools.hpp>
#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>

namespace bg2e::draw::pipeline {
namespace {
constexpr uint32_t MotionSettlingFrameMultiplier = 3;
static_assert(sizeof(glm::uvec4) == 16); // DiagnosticUniforms in both backend kernels.

struct alignas(16) TemporalUniforms {
    glm::mat4 inverseViewProjection{1}, previousViewProjection{1};
    glm::vec4 rejection{0}; // absolute/relative position, normal cosine, previous valid.
    glm::uvec4 counts{0}; // signal kind, frame limit, freeze, explicit world profile.
    glm::uvec4 extent{0}; // source and output dimensions.
    glm::vec4 cameraRoughness{0};
    glm::uvec4 flags{0}; // camera reprojection enabled.
};
struct alignas(16) TemporalMetadataUniforms {
    TemporalUniforms temporal;
    glm::mat4 currentViewProjection{1};
    glm::vec4 metadataRejection{0};
};
struct alignas(16) TemporalReprojectionUniforms {
    TemporalMetadataUniforms metadata;
    glm::mat4 previousView{1};
};
struct alignas(16) TemporalAdaptiveUniforms {
    TemporalReprojectionUniforms reprojection;
    glm::vec4 adaptive{0}; // minimum weight, reactive weight, clipping sigma (reserved), enabled.
};
static_assert(sizeof(TemporalAdaptiveUniforms) == 368 && offsetof(TemporalAdaptiveUniforms, adaptive) == 352);
static_assert(offsetof(TemporalAdaptiveUniforms, reprojection) == 0);
static_assert(offsetof(TemporalReprojectionUniforms, metadata) == 0);
static_assert(sizeof(TemporalReprojectionUniforms) == 352 && offsetof(TemporalReprojectionUniforms, previousView) == 288);
static_assert(sizeof(TemporalMetadataUniforms) == 288 && offsetof(TemporalMetadataUniforms, currentViewProjection) == 208);
static_assert(sizeof(TemporalUniforms) == 208 && offsetof(TemporalUniforms, flags) == 192);
}

TemporalAccumulator::TemporalAccumulator(TemporalConfiguration configuration)
    : _configuration(std::move(configuration))
{
    if (_configuration.shaderRoot.empty())
    {
        _configuration.shaderRoot = base::PlatformTools::shaderPath();
    }
}

void TemporalAccumulator::initialize(gpu::Device& device, gpu::BackendType backend, uint32_t slots)
{
    if (_device || !slots || !device.isValid() || !_configuration.maxHistories ||
        _configuration.shaderInterface != "draw.temporal.v1")
    {
        throw std::invalid_argument("Temporal initialization requires a clean lifecycle, valid slots/capacity/interface");
    }
    _device = &device;
    const auto usage = gpu::ImageUsage::Sampled | gpu::ImageUsage::Storage | gpu::ImageUsage::TransferSrc;
    for (auto format : {gpu::PixelFormat::R16_SFLOAT,gpu::PixelFormat::R16G16B16A16_SFLOAT,gpu::PixelFormat::R32G32B32A32_SFLOAT})
    {
        if (!device.formatCapabilities(format).supports(usage))
        {
            _status = "Temporal unavailable: required scalar/HDR/position storage formats are missing";
            return;
        }
    }
    try
    {
        gpu::PipelineLayoutDescription layout;
        layout.resourceBindings.push_back({0,{0,1},gpu::ResourceType::UniformBuffer,gpu::ShaderStage::Compute});
        for (uint32_t i = 1; i <= 8; ++i)
        {
            layout.resourceBindings.push_back({0,{i,i-1},gpu::ResourceType::SampledImage,gpu::ShaderStage::Compute});
        }
        layout.resourceBindings.push_back({0,{9,0},gpu::ResourceType::Sampler,gpu::ShaderStage::Compute});
        for (uint32_t i = 10; i <= 12; ++i)
        {
            layout.resourceBindings.push_back({0,{i,i-2},gpu::ResourceType::StorageImage,gpu::ShaderStage::Compute});
        }
        _layout = device.createPipelineLayout(layout);
        gpu::ShaderLib library(_configuration.shaderRoot,backend);
        for (uint32_t i = 0; i < 4; ++i)
        {
            const auto name = _configuration.shaderPrefix+"/"+(i >= 2 ? "world_" : "camera_")+(i%2 ? "hdr" : "scalar");
            _shaders[i] = library.compute(name,&device);
            _pipelines[i] = device.createComputePipeline({_shaders[i].get(),_layout.get()});
        }
        gpu::SamplerDescription nearest;
        nearest.minFilter = nearest.magFilter = gpu::Filter::Nearest;
        nearest.addressModeU = nearest.addressModeV = gpu::AddressMode::ClampToEdge;
        nearest.maxLod = 0;
        _nearest = device.createSampler(nearest);
        const float zero[4] = {};
        const uint16_t halfZero[4] = {};
        gpu::ImageDescription description;
        description.size = {1,1};
        description.usage = gpu::ImageUsage::Sampled | gpu::ImageUsage::TransferDst;
        description.format = gpu::PixelFormat::R32G32B32A32_SFLOAT;
        _zeroPosition = device.createImage(description);
        _zeroPosition->upload({zero,sizeof(zero),sizeof(zero),0,0});
        description.format = gpu::PixelFormat::R16G16B16A16_SFLOAT;
        _zeroNormal = device.createImage(description);
        _zeroNormal->upload({halfZero,sizeof(halfZero),sizeof(halfZero),0,0});
        _slots.resize(slots);
        _status = "Temporal accumulation ready";
        if (!_configuration.metadataShaderInterface.empty())
        {
            try
            {
                if (_configuration.metadataShaderInterface != "draw.temporal.v2" ||
                    !device.formatCapabilities(gpu::PixelFormat::R32_UINT).supports(gpu::ImageUsage::Sampled))
                {
                    throw std::runtime_error("Unsupported temporal metadata interface or integer sampling");
                }
                for (uint32_t i = 13; i <= 15; ++i)
                {
                    layout.resourceBindings.push_back({0,{i,i-2},gpu::ResourceType::SampledImage,gpu::ShaderStage::Compute});
                }
                _metadataLayout = device.createPipelineLayout(layout);
                for (uint32_t i = 0; i < 4; ++i)
                {
                    const auto name = _configuration.metadataShaderPrefix+"/"+(i >= 2 ? "world_" : "camera_")+(i%2 ? "hdr" : "scalar");
                    _metadataShaders[i] = library.compute(name,&device);
                    _metadataPipelines[i] = device.createComputePipeline({_metadataShaders[i].get(),_metadataLayout.get()});
                }
                _metadataStatus = "Optional draw.temporal.v2 ready";
                // Diagnostics are independently optional and never disable temporal kernels.
                try
                {
                    if (_configuration.metadataShaderPrefix != "draw/temporal_accumulation_v2")
                    {
                        throw std::runtime_error("Acceptance diagnostics require the built-in v2 shader family");
                    }
                    auto acceptanceLayout = layout;
                    acceptanceLayout.resourceBindings.push_back({0,{16,14},gpu::ResourceType::StorageImage,gpu::ShaderStage::Compute});
                    _acceptanceLayout = device.createPipelineLayout(acceptanceLayout);
                    for (uint32_t i = 0; i < 4; ++i)
                    {
                        const auto name = std::string("draw/temporal_accumulation_diagnostic/")+
                            (i >= 2 ? "world_" : "camera_")+(i%2 ? "hdr" : "scalar");
                        _acceptanceShaders[i] = library.compute(name,&device);
                        _acceptancePipelines[i] = device.createComputePipeline({_acceptanceShaders[i].get(),_acceptanceLayout.get()});
                    }
                    gpu::PipelineLayoutDescription diagnosticLayout;
                    diagnosticLayout.resourceBindings = {
                        {0,{0,1},gpu::ResourceType::UniformBuffer,gpu::ShaderStage::Compute},
                        {0,{1,0},gpu::ResourceType::SampledImage,gpu::ShaderStage::Compute},
                        {0,{2,1},gpu::ResourceType::SampledImage,gpu::ShaderStage::Compute},
                        {0,{3,0},gpu::ResourceType::Sampler,gpu::ShaderStage::Compute},
                        {0,{4,2},gpu::ResourceType::StorageImage,gpu::ShaderStage::Compute}};
                    _diagnosticLayout = device.createPipelineLayout(diagnosticLayout);
                    _diagnosticShader = library.compute("draw/temporal_diagnostic/view",&device);
                    _diagnosticPipeline = device.createComputePipeline({_diagnosticShader.get(),_diagnosticLayout.get()});
                    try
                    {
                        // Same display ABI, separately versioned shader: old diagnostic kernels remain unchanged.
                        _surfaceDiagnosticShader = library.compute("draw/temporal_diagnostic/surface",&device);
                        _surfaceDiagnosticPipeline = device.createComputePipeline({_surfaceDiagnosticShader.get(),_diagnosticLayout.get()});
                    }
                    catch (const std::exception& error)
                    {
                        _surfaceDiagnosticPipeline.reset();
                        _surfaceDiagnosticShader.reset();
                        _metadataStatus += std::string("; surface diagnostics unavailable: ")+error.what();
                    }
                }
                catch (const std::exception& error)
                {
                    _acceptancePipelines = {};
                    _acceptanceShaders = {};
                    _acceptanceLayout.reset();
                    _diagnosticPipeline.reset();
                    _diagnosticShader.reset();
                    _diagnosticLayout.reset();
                    _metadataStatus += std::string("; diagnostics unavailable: ")+error.what();
                }
            }
            catch (const std::exception& error)
            {
                _metadataPipelines = {};
                _metadataShaders = {};
                _metadataLayout.reset();
                _metadataStatus = std::string("Metadata unavailable; v1 fallback: ")+error.what();
            }
        }
        if (!_configuration.reprojectionShaderInterface.empty())
        {
            try
            {
                if (_configuration.reprojectionShaderInterface != "draw.temporal.v3" || !surfaceMetadataAvailable())
                {
                    throw std::runtime_error("Rigid reprojection requires draw.temporal.v3 and optional v2 metadata support");
                }
                auto reprojectionLayout = layout;
                for (uint32_t i = 17; i <= 18; ++i)
                {
                    reprojectionLayout.resourceBindings.push_back({0,{i,i-2},gpu::ResourceType::SampledImage,gpu::ShaderStage::Compute});
                }
                _reprojectionLayout = device.createPipelineLayout(reprojectionLayout);
                for (uint32_t i = 0; i < 2; ++i)
                {
                    const auto name = _configuration.reprojectionShaderPrefix+(i ? "/camera_hdr" : "/camera_scalar");
                    _reprojectionShaders[i] = library.compute(name,&device);
                    _reprojectionPipelines[i] = device.createComputePipeline({_reprojectionShaders[i].get(),_reprojectionLayout.get()});
                }
                _reprojectionStatus = "Optional draw.temporal.v3 camera-depth reprojection ready";
                try
                {
                    if (_configuration.reprojectionShaderPrefix != "draw/temporal_accumulation_v3" || !diagnosticsAvailable())
                    {
                        throw std::runtime_error("Rigid acceptance diagnostics require the built-in v3 family and display support");
                    }
                    reprojectionLayout.resourceBindings.push_back({0,{16,14},gpu::ResourceType::StorageImage,gpu::ShaderStage::Compute});
                    _reprojectionAcceptanceLayout = device.createPipelineLayout(reprojectionLayout);
                    for (uint32_t i = 0; i < 2; ++i)
                    {
                        const auto name = std::string("draw/temporal_accumulation_v3_diagnostic")+(i ? "/camera_hdr" : "/camera_scalar");
                        _reprojectionAcceptanceShaders[i] = library.compute(name,&device);
                        _reprojectionAcceptancePipelines[i] = device.createComputePipeline(
                            {_reprojectionAcceptanceShaders[i].get(),_reprojectionAcceptanceLayout.get()});
                    }
                }
                catch (const std::exception& error)
                {
                    _reprojectionAcceptancePipelines = {};
                    _reprojectionAcceptanceShaders = {};
                    _reprojectionAcceptanceLayout.reset();
                    _reprojectionStatus += std::string("; diagnostics unavailable: ")+error.what();
                }
            }
            catch (const std::exception& error)
            {
                _reprojectionPipelines = {};
                _reprojectionShaders = {};
                _reprojectionLayout.reset();
                _reprojectionStatus = std::string("Rigid reprojection unavailable; static v2/v1 fallback: ")+error.what();
            }
        }
        if (!_configuration.adaptiveShaderInterface.empty())
        {
            try
            {
                if (_configuration.adaptiveShaderInterface != "draw.temporal.v4" || !surfaceReprojectionAvailable())
                {
                    throw std::runtime_error("Adaptive accumulation requires draw.temporal.v4 and optional v3 reprojection support");
                }
                auto adaptiveLayout = layout;
                for (uint32_t i = 17; i <= 18; ++i)
                {
                    adaptiveLayout.resourceBindings.push_back({0,{i,i-2},gpu::ResourceType::SampledImage,gpu::ShaderStage::Compute});
                }
                _adaptiveLayout = device.createPipelineLayout(adaptiveLayout);
                for (uint32_t i = 0; i < 2; ++i)
                {
                    const auto name = _configuration.adaptiveShaderPrefix+(i ? "/camera_hdr" : "/camera_scalar");
                    _adaptiveShaders[i] = library.compute(name,&device);
                    _adaptivePipelines[i] = device.createComputePipeline({_adaptiveShaders[i].get(),_adaptiveLayout.get()});
                }
                _adaptiveStatus = "Optional draw.temporal.v4 bounded camera-depth weighting ready";
                try
                {
                    if (_configuration.adaptiveShaderPrefix != "draw/temporal_accumulation_v4" || !diagnosticsAvailable())
                    {
                        throw std::runtime_error("Adaptive acceptance diagnostics require the built-in v4 family and display support");
                    }
                    adaptiveLayout.resourceBindings.push_back({0,{16,14},gpu::ResourceType::StorageImage,gpu::ShaderStage::Compute});
                    _adaptiveAcceptanceLayout = device.createPipelineLayout(adaptiveLayout);
                    for (uint32_t i = 0; i < 2; ++i)
                    {
                        const auto name = std::string("draw/temporal_accumulation_v4_diagnostic")+(i ? "/camera_hdr" : "/camera_scalar");
                        _adaptiveAcceptanceShaders[i] = library.compute(name,&device);
                        _adaptiveAcceptancePipelines[i] = device.createComputePipeline(
                            {_adaptiveAcceptanceShaders[i].get(),_adaptiveAcceptanceLayout.get()});
                    }
                }
                catch (const std::exception& error)
                {
                    _adaptiveAcceptancePipelines = {};
                    _adaptiveAcceptanceShaders = {};
                    _adaptiveAcceptanceLayout.reset();
                    _adaptiveStatus += std::string("; diagnostics unavailable: ")+error.what();
                }
            }
            catch (const std::exception& error)
            {
                _adaptivePipelines = {};
                _adaptiveShaders = {};
                _adaptiveLayout.reset();
                _adaptiveStatus = std::string("Adaptive accumulation unavailable; nonadaptive v3 fallback: ")+error.what();
            }
        }
        if (!_configuration.momentShaderInterface.empty())
        {
            try
            {
                if (_configuration.momentShaderInterface != "draw.temporal.v5" || !adaptiveAvailable())
                {
                    throw std::runtime_error("Temporal moments require draw.temporal.v5 and optional v4 adaptive support");
                }
                const auto momentUsage = gpu::ImageUsage::Sampled | gpu::ImageUsage::Storage |
                    gpu::ImageUsage::TransferSrc | gpu::ImageUsage::TransferDst;
                if (!device.formatCapabilities(gpu::PixelFormat::R16G16_SFLOAT).supports(momentUsage))
                {
                    throw std::runtime_error("RG16F sampled/storage/transfer moments unsupported");
                }
                gpu::ImageDescription dummy;
                dummy.size = {1,1};
                dummy.format = gpu::PixelFormat::R16G16_SFLOAT;
                dummy.usage = gpu::ImageUsage::Sampled | gpu::ImageUsage::TransferDst;
                _zeroMoments = device.createImage(dummy);
                _zeroMoments->upload({halfZero,2*sizeof(uint16_t),2*sizeof(uint16_t),0,0});
                auto momentLayout = layout;
                for (uint32_t i = 17; i <= 18; ++i)
                {
                    momentLayout.resourceBindings.push_back({0,{i,i-2},gpu::ResourceType::SampledImage,gpu::ShaderStage::Compute});
                }
                momentLayout.resourceBindings.push_back({0,{19,17},gpu::ResourceType::SampledImage,gpu::ShaderStage::Compute});
                momentLayout.resourceBindings.push_back({0,{20,18},gpu::ResourceType::StorageImage,gpu::ShaderStage::Compute});
                _momentLayout = device.createPipelineLayout(momentLayout);
                for (uint32_t i = 0; i < 2; ++i)
                {
                    const auto name = _configuration.momentShaderPrefix+(i ? "/camera_hdr" : "/camera_scalar");
                    _momentShaders[i] = library.compute(name,&device);
                    _momentPipelines[i] = device.createComputePipeline({_momentShaders[i].get(),_momentLayout.get()});
                }
                _momentStatus = "Optional draw.temporal.v5 RG16F bounded-statistic moments ready";
                try
                {
                    if (_configuration.momentShaderPrefix != "draw/temporal_accumulation_v5" || !diagnosticsAvailable())
                    {
                        throw std::runtime_error("Moment acceptance diagnostics require the built-in v5 family and display support");
                    }
                    momentLayout.resourceBindings.push_back({0,{16,14},gpu::ResourceType::StorageImage,gpu::ShaderStage::Compute});
                    _momentAcceptanceLayout = device.createPipelineLayout(momentLayout);
                    for (uint32_t i = 0; i < 2; ++i)
                    {
                        const auto name = std::string("draw/temporal_accumulation_v5_diagnostic")+(i ? "/camera_hdr" : "/camera_scalar");
                        _momentAcceptanceShaders[i] = library.compute(name,&device);
                        _momentAcceptancePipelines[i] = device.createComputePipeline(
                            {_momentAcceptanceShaders[i].get(),_momentAcceptanceLayout.get()});
                    }
                }
                catch (const std::exception& error)
                {
                    _momentAcceptancePipelines = {};
                    _momentAcceptanceShaders = {};
                    _momentAcceptanceLayout.reset();
                    _momentStatus += std::string("; diagnostics unavailable: ")+error.what();
                }
            }
            catch (const std::exception& error)
            {
                _momentPipelines = {};
                _momentShaders = {};
                _momentLayout.reset();
                _zeroMoments.reset();
                _momentStatus = std::string("Temporal moments unavailable; neighborhood-only v4 fallback: ")+error.what();
            }
        }
        // Metric capture is a separate optional family; existing interfaces never receive binding 21.
        if (adaptiveAvailable())
        {
            try
            {
                gpu::PipelineLayoutDescription displayLayout;
                displayLayout.resourceBindings = {
                    {0,{0,1},gpu::ResourceType::UniformBuffer,gpu::ShaderStage::Compute},
                    {0,{1,0},gpu::ResourceType::SampledImage,gpu::ShaderStage::Compute},
                    {0,{2,1},gpu::ResourceType::SampledImage,gpu::ShaderStage::Compute},
                    {0,{3,0},gpu::ResourceType::Sampler,gpu::ShaderStage::Compute},
                    {0,{4,2},gpu::ResourceType::StorageImage,gpu::ShaderStage::Compute}};
                _metricDisplayLayout = device.createPipelineLayout(displayLayout);
                _metricDisplayShader = library.compute("draw/temporal_diagnostic/metrics",&device);
                _metricDisplayPipeline = device.createComputePipeline({_metricDisplayShader.get(),_metricDisplayLayout.get()});
                _adaptiveDiagnosticStatus = "Adaptive metric display ready";
                for (uint32_t version = 0; version < 2; ++version)
                {
                    try
                    {
                        if (version == 0 ? _configuration.adaptiveShaderPrefix != "draw/temporal_accumulation_v4" :
                            (!momentsAvailable() || _configuration.momentShaderPrefix != "draw/temporal_accumulation_v5"))
                        {
                            throw std::runtime_error("Metric capture requires the corresponding canonical available family");
                        }
                        auto metricLayout = layout;
                        for (uint32_t binding = 17; binding <= 18; ++binding)
                        {
                            metricLayout.resourceBindings.push_back({0,{binding,binding-2},gpu::ResourceType::SampledImage,gpu::ShaderStage::Compute});
                        }
                        if (version == 1)
                        {
                            metricLayout.resourceBindings.push_back({0,{19,17},gpu::ResourceType::SampledImage,gpu::ShaderStage::Compute});
                            metricLayout.resourceBindings.push_back({0,{20,18},gpu::ResourceType::StorageImage,gpu::ShaderStage::Compute});
                        }
                        metricLayout.resourceBindings.push_back({0,{21,19},gpu::ResourceType::StorageImage,gpu::ShaderStage::Compute});
                        _metricLayouts[version] = device.createPipelineLayout(metricLayout);
                        for (uint32_t profile = 0; profile < 2; ++profile)
                        {
                            const auto name = std::string(version ? "draw/temporal_accumulation_v5_metrics" :
                                "draw/temporal_accumulation_v4_metrics")+(profile ? "/camera_hdr" : "/camera_scalar");
                            const auto index = 2*version+profile;
                            _metricShaders[index] = library.compute(name,&device);
                            _metricPipelines[index] = device.createComputePipeline({_metricShaders[index].get(),_metricLayouts[version].get()});
                        }
                        _adaptiveDiagnosticStatus += version ? "; v5 capture ready" : "; v4 capture ready";
                    }
                    catch (const std::exception& error)
                    {
                        _metricLayouts[version].reset();
                        for (uint32_t profile = 0; profile < 2; ++profile)
                        {
                            _metricPipelines[2*version+profile].reset();
                            _metricShaders[2*version+profile].reset();
                        }
                        _adaptiveDiagnosticStatus += std::string(version ? "; v5 capture unavailable: " : "; v4 capture unavailable: ")+error.what();
                    }
                }
            }
            catch (const std::exception& error)
            {
                _metricDisplayPipeline.reset();
                _metricDisplayShader.reset();
                _metricDisplayLayout.reset();
                _metricPipelines = {};
                _metricShaders = {};
                _metricLayouts = {};
                _adaptiveDiagnosticStatus = std::string("Adaptive metric diagnostics unavailable: ")+error.what();
            }
        }

    }
    catch (const std::exception& error)
    {
        _pipelines = {};
        _shaders = {};
        _layout.reset();
        _nearest.reset();
        _zeroPosition.reset();
        _zeroNormal.reset();
        _slots.clear();
        _status = std::string("Temporal unavailable: ")+error.what();
    }
}

void TemporalAccumulator::beginFrame(uint32_t slotIndex, uint64_t serial, uint64_t sequence, bool sceneMotion)
{
    if (!available())
    {
        return;
    }
    if (_hasSequence && sequence < _latestSequence)
    {
        throw std::logic_error("Temporal scene sequence cannot move backwards within a lifecycle");
    }
    auto& slot = _slots.at(slotIndex);
    if (slot.prepared && slot.serial == serial)
    {
        throw std::logic_error("Begin temporal acquired slot only once per serial");
    }
    _latestSequence = sequence;
    _hasSequence = true;
    slot = {};
    slot.serial = serial;
    slot.sequence = sequence;
    slot.sceneMotion = sceneMotion;
    slot.prepared = true;
    for (auto it = _histories.begin(); it != _histories.end();)
    {
        if (it->second->sequence < sequence && sequence-it->second->sequence > 1)
        {
            it = _histories.erase(it);
        }
        else
        {
            ++it;
        }
    }
    if (_pool)
    {
        _pool->collect();
    }

}

TemporalResult TemporalAccumulator::record(const TemporalInput& input)
{
    base::CpuProfileScope profile("effect.temporal.record");
    if (input.diagnoseAcceptance && input.diagnoseAdaptive)
    {
        throw std::invalid_argument("Select only one temporal capture kind per invocation");
    }
    if (!_device || input.command.hasActiveScope() || input.signal.device() != _device || !input.signal.isValid() ||
        input.signal.size().isZero() || input.signal.imageType() != gpu::ImageType::Image2D ||
        !gpu::hasFlag(input.signal.usage(),gpu::ImageUsage::Sampled) ||
        uint32_t(input.key.signal) > uint32_t(TemporalSignal::ReflectedRadiance))
    {
        throw std::invalid_argument("Temporal input requires a valid same-device sampled signal and inactive scope");
    }
    const bool scalar = input.key.signal == TemporalSignal::Visibility;
    if (input.signal.pixelFormat() != (scalar ? gpu::PixelFormat::R16_SFLOAT : gpu::PixelFormat::R16G16B16A16_SFLOAT))
    {
        throw std::invalid_argument("Temporal signal kind does not match scalar/HDR format");
    }
    if (bool(input.metadata.token) != bool(input.metadata.depth))
    {
        throw std::invalid_argument("Temporal metadata requires both retained token and depth");
    }
    if (input.metadata.token)
    {
        input.surface.validate(*_device);
        for (const auto& image : {input.metadata.token,input.metadata.depth})
        {
            if (!image->isValid() || image->device() != _device || image->imageType() != gpu::ImageType::Image2D ||
                image->size() != input.surface.extent() || !gpu::hasFlag(image->usage(),gpu::ImageUsage::Sampled))
            {
                throw std::invalid_argument("Temporal metadata requires matching same-device sampled receiver images");
            }
        }
        const auto depthFormat = input.metadata.depth->pixelFormat();
        if (input.metadata.token->pixelFormat() != gpu::PixelFormat::R32_UINT ||
            (depthFormat != gpu::PixelFormat::D16_UNORM && depthFormat != gpu::PixelFormat::D32_SFLOAT) ||
            (input.surface.profile == SurfaceProfile::CameraDepth && depthFormat != input.surface.depthOrPosition->pixelFormat()))
        {
            throw std::invalid_argument("Temporal metadata requires R32_UINT token and matching receiver depth format");
        }
    }
    if (bool(input.metadata.previousPosition) != bool(input.metadata.previousNormal) ||
        (input.metadata.previousPosition && !input.metadata.token))
    {
        throw std::invalid_argument("Previous surface metadata requires a retained position/normal pair plus token/depth");
    }
    if (input.metadata.previousPosition)
    {
        for (const auto& image : {input.metadata.previousPosition,input.metadata.previousNormal})
        {
            if (!image->isValid() || image->device() != _device || image->imageType() != gpu::ImageType::Image2D ||
                image->size() != input.surface.extent() || !gpu::hasFlag(image->usage(),gpu::ImageUsage::Sampled))
            {
                throw std::invalid_argument("Previous surface images require matching same-device sampled receiver images");
            }
        }
        if (input.metadata.previousPosition->pixelFormat() != gpu::PixelFormat::R32G32B32A32_SFLOAT ||
            input.metadata.previousNormal->pixelFormat() != gpu::PixelFormat::R16G16B16A16_SFLOAT)
        {
            throw std::invalid_argument("Previous surface images require RGBA32F position and RGBA16F normal");
        }
    }
    const bool affineView = input.view.view[0][3] == 0 && input.view.view[1][3] == 0 &&
        input.view.view[2][3] == 0 && input.view.view[3][3] == 1;
    const bool rigid = input.metadata.previousPosition && surfaceReprojectionAvailable() && affineView &&
        input.surface.profile == SurfaceProfile::CameraDepth && input.view.reproject;
    const bool enhanced = input.metadata.token && surfaceMetadataAvailable() && (rigid || input.metadata.legacyCompatible);
    const bool adaptive = rigid && _settings.adaptiveEnabled && adaptiveAvailable();
    const bool moments = adaptive && momentsAvailable();
    const uint32_t metadataMode = moments ? 4u : (adaptive ? 3u : (rigid ? 2u : (enhanced ? 1u : 0u)));
    const bool diagnose = enhanced && input.diagnoseAcceptance && diagnosticsAvailable() &&
        (moments ? bool(_momentAcceptancePipelines[0]) : (adaptive ? bool(_adaptiveAcceptancePipelines[0]) : (!rigid || bool(_reprojectionAcceptancePipelines[0]))));
    const uint32_t metricIndex = (moments ? 2u : 0u)+(scalar ? 0u : 1u);
    const bool metrics = adaptive && input.diagnoseAdaptive && _metricDisplayPipeline && bool(_metricPipelines[metricIndex]);
    if (!available() || !_settings.enabled)
    {
        _histories.erase(input.key);
        return {&input.signal,{},1};
    }
    auto& slot = _slots.at(input.slot);
    if (!slot.prepared || slot.serial != input.serial || slot.sequence != input.sceneFrameNumber)
    {
        throw std::logic_error("Begin the acquired temporal scene slot before recording");
    }
    input.surface.validate(*_device);
    MotionVectorGenerator::validate(input.view);
    if (input.key.signal == TemporalSignal::ReflectedRadiance && (!input.material || !input.material->isValid() ||
        input.material->device() != _device || input.material->size() != input.surface.extent() ||
        input.material->imageType() != gpu::ImageType::Image2D || !gpu::hasFlag(input.material->usage(),gpu::ImageUsage::Sampled) ||
        (input.material->pixelFormat() != gpu::PixelFormat::R8G8B8A8_UNORM && input.material->pixelFormat() != gpu::PixelFormat::R16G16B16A16_SFLOAT) ||
        !std::isfinite(input.maxRoughness) || input.maxRoughness < 0 || input.maxRoughness > 1))
    {
        throw std::invalid_argument("Temporal reflections require matching roughness and threshold");
    }
    auto found = _histories.find(input.key);
    auto previous = found != _histories.end() ? found->second : nullptr;
    if (previous && (!previous->recording ||
        (!previous->recording->wasSubmitted() && previous->recording != input.command.recordingCompletion())))
    {
        previous.reset(); // A different unsubmitted/abandoned recording is not history.
    }
    if (previous && previous->sequence == input.sceneFrameNumber)
    {
        throw std::logic_error("Record each effect/view/layer once per rendered scene sequence");
    }
    const bool consecutive = previous && previous->sequence < input.sceneFrameNumber &&
        input.sceneFrameNumber-previous->sequence == 1;
    // Legacy paths retain the unjittered camera rejection. Supported v3 uses
    // geometric reprojection; projection changes and explicit cuts still reset.
    bool cameraChanged = false;
    if (previous)
    {
        const auto before = previous->view.projection * previous->view.view;
        const auto current = input.view.projection * input.view.view;
        for (int column = 0; column < 4; ++column)
        {
            for (int row = 0; row < 4; ++row)
            {
                cameraChanged = cameraChanged || std::abs(before[column][row] - current[column][row]) > 0.001f;
            }
        }
    }
    if (!consecutive || (cameraChanged && !rigid) || previous->metadataMode != metadataMode ||
        previous->generation != input.generation || previous->reset != _generation ||
        previous->signal->size() != input.signal.size() || previous->surfaceExtent != input.surface.extent() ||
        previous->profile != input.surface.profile || previous->view.reproject != input.view.reproject ||
        previous->view.projection != input.view.projection || previous->maxRoughness != input.maxRoughness)
    {
        previous.reset();
    }
    if (moments && previous && !previous->moments)
    {
        previous.reset();
    }
    if (found == _histories.end() && _histories.size() >= _configuration.maxHistories)
    {
        throw std::length_error("Temporal effect/view/layer history capacity exceeded");
    }
    // Only explicit, available acceptance captures receive these extra timing labels.
    std::optional<base::CpuProfileScope> diagnosticCpu;
    std::optional<gpu::GpuTimingScope> diagnosticGpu;
    if (diagnose || metrics)
    {
        diagnosticCpu.emplace("effect.temporal.diagnosticCapture");
        diagnosticGpu.emplace(input.command,"effect.temporal.diagnosticCapture",input.key.layer,input.key.layer != 0);
    }
    if (!_pool)
    {
        _pool = std::make_shared<TemporaryResourcePool>(*_device, _configuration.poolLimits);
    }
    _pool->setEnabled(_poolingEnabled);
    auto next = std::make_shared<History>();
    next->recording = input.command.recordingCompletion();
    next->sequence = input.sceneFrameNumber;
    next->generation = input.generation;
    next->reset = _generation;
    next->view = input.view;
    next->metadataMode = metadataMode;
    if (enhanced)
    {
        next->metadata = input.metadata;
        next->metadata.motion.reset(); // Display invocations retain motion only when they actually read it.
    }
    next->profile = input.surface.profile;
    next->surfaceExtent = input.surface.extent();
    next->maxRoughness = input.maxRoughness;
    next->count = previous ? std::min(_settings.maxFrames,previous->count+1) : 1;
    // Only a validated consecutive predecessor may carry a settling window.
    // The last frame decrementing 1 to 0 still rolls; the following frame freezes.
    const bool rollingFromPrevious = previous && previous->rollingFramesRemaining > 0;
    next->rollingFramesRemaining = slot.sceneMotion ? MotionSettlingFrameMultiplier * _settings.maxFrames :
        (rollingFromPrevious ? previous->rollingFramesRemaining - 1 : 0);
    const bool effectiveFreeze = _settings.freezeAtLimit && !slot.sceneMotion && !rollingFromPrevious;
    gpu::ImageDescription description;
    description.size = input.signal.size();
    description.usage = gpu::ImageUsage::Sampled | gpu::ImageUsage::Storage | gpu::ImageUsage::TransferSrc;
    description.format = input.signal.pixelFormat();
    next->signal = _pool->image(input.command, description);
    description.format = gpu::PixelFormat::R32G32B32A32_SFLOAT;
    next->position = _pool->image(input.command, description);
    description.format = gpu::PixelFormat::R16G16B16A16_SFLOAT;
    next->normal = _pool->image(input.command, description);
    if (diagnose)
    {
        next->acceptance = _pool->image(input.command,description);
    }
    if (metrics)
    {
        next->metrics = _pool->image(input.command,description); // Existing RGBA16F guide format.
    }
    if (moments)
    {
        description.format = gpu::PixelFormat::R16G16_SFLOAT;
        next->moments = _pool->image(input.command,description);
    }
    Invocation invocation;
    invocation.previous = previous;
    invocation.next = next;
    TemporalAdaptiveUniforms adaptiveData;
    auto& reprojectionData = adaptiveData.reprojection;
    adaptiveData.adaptive = {_settings.minCurrentWeight,_settings.reactiveCurrentWeight,_settings.historyClipSigma,adaptive ? 1.0f : 0.0f};
    auto& enhancedData = reprojectionData.metadata;
    auto& data = enhancedData.temporal;
    data.inverseViewProjection = input.surface.inverseViewProjection;
    data.previousViewProjection = previous ? previous->view.jitteredProjection*previous->view.view : glm::mat4(1);
    data.rejection = {_settings.absolutePositionThreshold,_settings.relativePositionThreshold,_settings.normalThreshold,previous ? 1.0f : 0.0f};
    data.counts = {uint32_t(input.key.signal),_settings.maxFrames,effectiveFreeze ? 1u : 0u,
        input.surface.profile == SurfaceProfile::WorldPositionCoverage ? 1u : 0u};
    data.extent = {input.surface.extent().width,input.surface.extent().height,input.signal.width(),input.signal.height()};
    data.cameraRoughness = glm::vec4(glm::vec3(glm::inverse(input.view.view)[3]),input.maxRoughness);
    data.flags.x = input.view.reproject ? 1u : 0u;
    data.flags.y = enhanced ? 1u : 0u;
    data.flags.z = enhanced && previous && previous->metadata.token ? 1u : 0u;
    enhancedData.currentViewProjection = input.view.jitteredProjection*input.view.view;
    enhancedData.metadataRejection.x = enhanced && input.metadata.depth->pixelFormat() == gpu::PixelFormat::D16_UNORM ?
        2.0f/65535.0f : 2e-6f;
    reprojectionData.previousView = previous ? previous->view.view : glm::mat4(1);
    const auto uniformBytes = adaptive ? sizeof(adaptiveData) : (rigid ? sizeof(reprojectionData) : (enhanced ? sizeof(enhancedData) : sizeof(data)));
    const auto selectedLayout = metrics ? _metricLayouts[moments ? 1u : 0u] : moments ? (diagnose ? _momentAcceptanceLayout : _momentLayout) : (adaptive ? (diagnose ? _adaptiveAcceptanceLayout : _adaptiveLayout) : (rigid ? (diagnose ? _reprojectionAcceptanceLayout : _reprojectionLayout) :
        (diagnose ? _acceptanceLayout : (enhanced ? _metadataLayout : _layout))));
    invocation.pooled = _pool->bindings(input.command, selectedLayout, 0, uniformBytes,
        {input.slot, uint64_t(input.key.signal), input.key.view, input.key.layer});
    invocation.pooled->writeUniforms(&adaptiveData, uniformBytes);
    invocation.uniforms = invocation.pooled->uniforms();
    invocation.resources = invocation.pooled->resources();
    invocation.pooled->setUniformBuffer({0,1},invocation.uniforms.get());
    auto* coverage = input.surface.profile == SurfaceProfile::CameraDepth ? input.surface.normal : input.surface.coverage;
    const std::array<gpu::Image*,8> sources{input.surface.normal,input.surface.depthOrPosition,coverage,&input.signal,
        previous ? previous->signal.get() : &input.signal,previous ? previous->position.get() : _zeroPosition.get(),
        previous ? previous->normal.get() : _zeroNormal.get(),input.key.signal == TemporalSignal::ReflectedRadiance ? input.material : input.surface.normal};
    for (uint32_t i = 0; i < sources.size(); ++i)
    {
        input.command.transition(sources[i],gpu::ImageLayout::ShaderReadOnly);
        invocation.pooled->setSampledImage({i+1,i},sources[i]);
    }
    if (enhanced)
    {
        // Missing previous metadata binds a typed current token, guarded by flags.z.
        const std::array<gpu::Image*,3> metadataSources{input.metadata.token.get(),input.metadata.depth.get(),
            previous && previous->metadata.token ? previous->metadata.token.get() : input.metadata.token.get()};
        for (uint32_t i = 0; i < metadataSources.size(); ++i)
        {
            input.command.transition(metadataSources[i],gpu::ImageLayout::ShaderReadOnly);
            invocation.pooled->setSampledImage({i+13,i+11},metadataSources[i]);
        }
    }
    if (rigid)
    {
        const std::array<gpu::Image*,2> expectedSources{input.metadata.previousPosition.get(),input.metadata.previousNormal.get()};
        for (uint32_t i = 0; i < expectedSources.size(); ++i)
        {
            input.command.transition(expectedSources[i],gpu::ImageLayout::ShaderReadOnly);
            invocation.pooled->setSampledImage({i+17,i+15},expectedSources[i]);
        }
    }
    if (moments)
    {
        auto* previousMoments = previous ? previous->moments.get() : _zeroMoments.get();
        input.command.transition(previousMoments,gpu::ImageLayout::ShaderReadOnly);
        invocation.pooled->setSampledImage({19,17},previousMoments);
        input.command.transition(next->moments.get(),gpu::ImageLayout::General);
        invocation.pooled->setStorageImage({20,18},next->moments.get());
    }
    invocation.pooled->setSampler({9,0},_nearest.get());
    const std::array<gpu::Image*,3> outputs{next->signal.get(),next->position.get(),next->normal.get()};
    for (uint32_t i = 0; i < outputs.size(); ++i)
    {
        input.command.transition(outputs[i],gpu::ImageLayout::General);
        invocation.pooled->setStorageImage({i+10,i+8},outputs[i]);
    }
    if (diagnose)
    {
        input.command.transition(next->acceptance.get(),gpu::ImageLayout::General);
        invocation.pooled->setStorageImage({16,14},next->acceptance.get());
    }
    if (metrics)
    {
        input.command.transition(next->metrics.get(),gpu::ImageLayout::General);
        invocation.pooled->setStorageImage({21,19},next->metrics.get());
    }
    invocation.pooled->update();
    slot.invocations.push_back(std::move(invocation));
    const uint32_t index = (input.surface.profile == SurfaceProfile::WorldPositionCoverage ? 2u : 0u)+(scalar ? 0u : 1u);
    input.command.beginCompute();
    auto* pipeline = metrics ? _metricPipelines[metricIndex].get() : moments ? (diagnose ? _momentAcceptancePipelines[index].get() : _momentPipelines[index].get()) : (adaptive ? (diagnose ? _adaptiveAcceptancePipelines[index].get() : _adaptivePipelines[index].get()) : (rigid ? (diagnose ? _reprojectionAcceptancePipelines[index].get() : _reprojectionPipelines[index].get()) :
        (diagnose ? _acceptancePipelines[index].get() : (enhanced ? _metadataPipelines[index].get() : _pipelines[index].get()))));
    input.command.bindPipeline(pipeline);
    input.command.bindResourceSet(pipeline,0,slot.invocations.back().resources.get());
    input.command.dispatch((input.signal.width()+15)/16,(input.signal.height()+15)/16,1);
    input.command.endCompute();
    for (auto* image : outputs)
    {
        input.command.transition(image,gpu::ImageLayout::ShaderReadOnly);
    }
    if (next->acceptance)
    {
        input.command.transition(next->acceptance.get(),gpu::ImageLayout::ShaderReadOnly);
    }
    if (next->moments)
    {
        input.command.transition(next->moments.get(),gpu::ImageLayout::ShaderReadOnly);
    }
    if (next->metrics)
    {
        input.command.transition(next->metrics.get(),gpu::ImageLayout::ShaderReadOnly);
    }
    _histories[input.key] = next;
    return {next->signal.get(),next->signal,next->count};
}

bool TemporalAccumulator::diagnosticsAvailable() const
{
    return surfaceMetadataAvailable() && bool(_diagnosticPipeline) && bool(_acceptancePipelines[0]);
}

bool TemporalAccumulator::surfaceDiagnosticsAvailable() const
{
    return surfaceMetadataAvailable() && bool(_surfaceDiagnosticPipeline);
}

bool TemporalAccumulator::adaptiveDiagnosticsAvailable() const
{
    return bool(_metricDisplayPipeline) && (bool(_metricPipelines[0]) || bool(_metricPipelines[2]));
}

const std::string& TemporalAccumulator::adaptiveDiagnosticsStatus() const
{
    return _adaptiveDiagnosticStatus;
}

std::shared_ptr<gpu::Image> TemporalAccumulator::recordDiagnostic(gpu::CommandBuffer& command,
    uint32_t slotIndex, uint64_t serial, const TemporalKey& key,
    const TemporalSurfaceMetadata& metadata, TemporalDiagnosticView view)
{
    if (uint32_t(view) > uint32_t(TemporalDiagnosticView::EffectiveCount))
    {
        throw std::invalid_argument("Unknown temporal diagnostic view");
    }
    const bool metricView = uint32_t(view) >= uint32_t(TemporalDiagnosticView::CurrentWeight);
    const bool surfaceView = uint32_t(view) >= uint32_t(TemporalDiagnosticView::Motion) &&
        uint32_t(view) <= uint32_t(TemporalDiagnosticView::PreviousNormal);
    if (view == TemporalDiagnosticView::Off ||
        !(metricView ? adaptiveDiagnosticsAvailable() : (surfaceView ? surfaceDiagnosticsAvailable() : diagnosticsAvailable())) || !metadata.token || !metadata.depth)
    {
        return {};
    }
    auto& slot = _slots.at(slotIndex);
    if (command.hasActiveScope() || !slot.prepared || slot.serial != serial)
    {
        throw std::logic_error("Temporal diagnostics require the acquired prepared scene slot");
    }
    if (!metadata.token->isValid() || metadata.token->device() != _device ||
        metadata.token->imageType() != gpu::ImageType::Image2D || metadata.token->size().isZero() ||
        metadata.token->pixelFormat() != gpu::PixelFormat::R32_UINT ||
        !gpu::hasFlag(metadata.token->usage(),gpu::ImageUsage::Sampled))
    {
        throw std::invalid_argument("Temporal diagnostic token must be a same-device sampled R32_UINT 2D image");
    }
    std::shared_ptr<gpu::Image> surfaceData;
    if (surfaceView)
    {
        surfaceData = view == TemporalDiagnosticView::Motion ? metadata.motion :
            (view == TemporalDiagnosticView::PreviousPosition ? metadata.previousPosition : metadata.previousNormal);
        if (!surfaceData)
        {
            return {}; // Missing required data is unavailable, never a valid zero-vector display.
        }
        const auto format = view == TemporalDiagnosticView::Motion ? gpu::PixelFormat::R16G16_SFLOAT :
            (view == TemporalDiagnosticView::PreviousPosition ? gpu::PixelFormat::R32G32B32A32_SFLOAT : gpu::PixelFormat::R16G16B16A16_SFLOAT);
        if (!surfaceData->isValid() || surfaceData->device() != _device || surfaceData->size() != metadata.token->size() ||
            surfaceData->imageType() != gpu::ImageType::Image2D || surfaceData->mipLevels() != 1 || surfaceData->arrayLayers() != 1 ||
            surfaceData->pixelFormat() != format || !gpu::hasFlag(surfaceData->usage(),gpu::ImageUsage::Sampled))
        {
            throw std::invalid_argument("Temporal surface display requires matching sampled receiver metadata of the selected format");
        }
    }
    // Off, missing data and unavailable diagnostic paths return before this scope.
    base::CpuProfileScope diagnosticCpu("effect.temporal.diagnosticDisplay");
    gpu::GpuTimingScope diagnosticGpu(command,"effect.temporal.diagnosticDisplay",key.layer,key.layer != 0);
    if (!_pool)
    {
        _pool = std::make_shared<TemporaryResourcePool>(*_device,_configuration.poolLimits);
    }
    _pool->setEnabled(_poolingEnabled);
    Invocation invocation;
    const auto found = _histories.find(key);
    if (found != _histories.end() && found->second->sequence == slot.sequence &&
        found->second->recording == command.recordingCompletion() && found->second->metadata.token == metadata.token &&
        (metricView ? bool(found->second->metrics) : bool(found->second->acceptance)))
    {
        invocation.next = found->second;
    }
    invocation.diagnosticMetadata = metadata;
    if (view != TemporalDiagnosticView::Motion)
    {
        invocation.diagnosticMetadata.motion.reset();
    }
    gpu::ImageDescription description;
    description.size = metadata.token->size();
    description.format = gpu::PixelFormat::R16G16B16A16_SFLOAT;
    description.usage = gpu::ImageUsage::Sampled | gpu::ImageUsage::Storage | gpu::ImageUsage::TransferSrc;
    invocation.diagnostic = _pool->image(command,description);
    // 16-byte uniform: receiver width/height, mode, current acceptance image present.
    const glm::uvec4 data(description.size.width,description.size.height,uint32_t(view),(surfaceView || invocation.next) ? 1u : 0u);
    const auto displayLayout = metricView ? _metricDisplayLayout : _diagnosticLayout;
    invocation.pooled = _pool->bindings(command,displayLayout,0,sizeof(data),
        {slotIndex,uint64_t(key.signal),key.view,key.layer});
    invocation.pooled->writeUniforms(&data,sizeof(data));
    invocation.pooled->setUniformBuffer({0,1},invocation.pooled->uniforms().get());
    auto* acceptance = surfaceView ? surfaceData.get() : (invocation.next ?
        (metricView ? invocation.next->metrics.get() : invocation.next->acceptance.get()) : _zeroNormal.get());
    command.transition(metadata.token.get(),gpu::ImageLayout::ShaderReadOnly);
    command.transition(acceptance,gpu::ImageLayout::ShaderReadOnly);
    command.transition(invocation.diagnostic.get(),gpu::ImageLayout::General);
    invocation.pooled->setSampledImage({1,0},metadata.token.get());
    invocation.pooled->setSampledImage({2,1},acceptance);
    invocation.pooled->setSampler({3,0},_nearest.get());
    invocation.pooled->setStorageImage({4,2},invocation.diagnostic.get());
    invocation.pooled->update();
    const auto result = invocation.diagnostic;
    slot.invocations.push_back(std::move(invocation));
    command.beginCompute();
    auto* pipeline = metricView ? _metricDisplayPipeline.get() : (surfaceView ? _surfaceDiagnosticPipeline.get() : _diagnosticPipeline.get());
    command.bindPipeline(pipeline);
    command.bindResourceSet(pipeline,0,slot.invocations.back().pooled->resources().get());
    command.dispatch((description.size.width+15)/16,(description.size.height+15)/16,1);
    command.endCompute();
    command.transition(result.get(),gpu::ImageLayout::ShaderReadOnly);
    return result;
}

void TemporalAccumulator::invalidate()
{
    if (_generation == std::numeric_limits<uint64_t>::max())
    {
        throw std::overflow_error("Temporal reset generation exhausted");
    }
    ++_generation;
    _histories.clear();
}

void TemporalAccumulator::cleanup()
{
    if (_device)
    {
        _device->waitIdleForResourceUpdate();
    }
    _histories.clear();
    _slots.clear();
    _pool.reset();
    _metricPipelines = {};
    _metricShaders = {};
    _metricLayouts = {};
    _metricDisplayPipeline.reset();
    _metricDisplayShader.reset();
    _metricDisplayLayout.reset();
    _adaptiveDiagnosticStatus = "Optional adaptive metrics unavailable";
    _momentPipelines = {};
    _momentShaders = {};
    _momentLayout.reset();
    _momentAcceptancePipelines = {};
    _momentAcceptanceShaders = {};
    _momentAcceptanceLayout.reset();
    _zeroMoments.reset();
    _momentStatus = "Optional moment interface not requested";
    _adaptivePipelines = {};
    _adaptiveShaders = {};
    _adaptiveLayout.reset();
    _adaptiveAcceptancePipelines = {};
    _adaptiveAcceptanceShaders = {};
    _adaptiveAcceptanceLayout.reset();
    _adaptiveStatus = "Optional adaptive interface not requested";
    _reprojectionPipelines = {};
    _reprojectionShaders = {};
    _reprojectionLayout.reset();
    _reprojectionAcceptancePipelines = {};
    _reprojectionAcceptanceShaders = {};
    _reprojectionAcceptanceLayout.reset();
    _reprojectionStatus = "Optional reprojection interface not requested";
    _acceptancePipelines = {};
    _acceptanceShaders = {};
    _acceptanceLayout.reset();
    _surfaceDiagnosticPipeline.reset();
    _surfaceDiagnosticShader.reset();
    _diagnosticPipeline.reset();
    _diagnosticShader.reset();
    _diagnosticLayout.reset();
    _metadataPipelines = {};
    _metadataShaders = {};
    _metadataLayout.reset();
    _metadataStatus = "Optional metadata interface not requested";
    _pipelines = {};
    _shaders = {};
    _layout.reset();
    _nearest.reset();
    _zeroPosition.reset();
    _zeroNormal.reset();
    _device = nullptr;
    _hasSequence = false;
    _latestSequence = 0;
    _status = "Not initialized";
}

const TemporalSettings& TemporalAccumulator::settings() const
{
    return _settings;
}

void TemporalAccumulator::setEnabled(bool value)
{
    if (_settings.enabled != value)
    {
        _settings.enabled = value;
        invalidate();
    }
}

void TemporalAccumulator::setMaxFrames(uint32_t value)
{
    if (!value || value > 1024)
    {
        throw std::invalid_argument("Temporal frame bound must be 1..1024");
    }
    if (_settings.maxFrames != value)
    {
        _settings.maxFrames = value;
        invalidate();
    }
}

void TemporalAccumulator::setFreezeAtLimit(bool value)
{
    if (_settings.freezeAtLimit != value)
    {
        _settings.freezeAtLimit = value;
        invalidate();
    }
}

void TemporalAccumulator::setPositionThresholds(float absolute, float relative)
{
    if (!std::isfinite(absolute) || !std::isfinite(relative) || absolute < 0 || relative < 0)
    {
        throw std::invalid_argument("Temporal position thresholds must be finite and nonnegative");
    }
    _settings.absolutePositionThreshold = absolute;
    _settings.relativePositionThreshold = relative;
    invalidate();
}

void TemporalAccumulator::setNormalThreshold(float value)
{
    if (!std::isfinite(value) || value < -1 || value > 1)
    {
        throw std::invalid_argument("Temporal normal threshold must be in [-1,1]");
    }
    _settings.normalThreshold = value;
    invalidate();
}

void TemporalAccumulator::validateAdaptiveSettings(bool, float minimum, float reactive, float sigma)
{
    if (!std::isfinite(minimum) || !std::isfinite(reactive) || !std::isfinite(sigma) ||
        minimum < 0 || minimum > 1 || reactive < minimum || reactive > 1 || sigma < 0 || sigma > 10)
    {
        throw std::invalid_argument("Adaptive weights require 0 <= minimum <= reactive <= 1 and sigma in [0,10]");
    }
}

void TemporalAccumulator::setAdaptiveSettings(bool enabled, float minimum, float reactive, float sigma)
{
    validateAdaptiveSettings(enabled,minimum,reactive,sigma);
    if (_settings.adaptiveEnabled != enabled || _settings.minCurrentWeight != minimum ||
        _settings.reactiveCurrentWeight != reactive || _settings.historyClipSigma != sigma)
    {
        invalidate();
        _settings.adaptiveEnabled = enabled;
        _settings.minCurrentWeight = minimum;
        _settings.reactiveCurrentWeight = reactive;
        _settings.historyClipSigma = sigma;
    }
}

bool TemporalAccumulator::momentsAvailable() const
{
    return adaptiveAvailable() && bool(_momentPipelines[0]) && bool(_zeroMoments);
}

const std::string& TemporalAccumulator::momentsStatus() const
{
    return _momentStatus;
}

bool TemporalAccumulator::adaptiveAvailable() const
{
    return surfaceReprojectionAvailable() && bool(_adaptivePipelines[0]);
}

const std::string& TemporalAccumulator::adaptiveStatus() const
{
    return _adaptiveStatus;
}

bool TemporalAccumulator::available() const
{
    return bool(_pipelines[0]) && !_slots.empty();
}

bool TemporalAccumulator::surfaceMetadataAvailable() const
{
    return available() && bool(_metadataPipelines[0]);
}

bool TemporalAccumulator::surfaceReprojectionAvailable() const
{
    return surfaceMetadataAvailable() && bool(_reprojectionPipelines[0]);
}

const std::string& TemporalAccumulator::surfaceReprojectionStatus() const
{
    return _reprojectionStatus;
}

const std::string& TemporalAccumulator::surfaceMetadataStatus() const
{
    return _metadataStatus;
}

const std::string& TemporalAccumulator::status() const
{
    return _status;
}

uint64_t TemporalAccumulator::generation() const
{
    return _generation;
}

uint32_t TemporalAccumulator::frameCount(const TemporalKey& key) const
{
    const auto found = _histories.find(key);
    return found != _histories.end() ? found->second->count : 0;
}

TemporaryPoolStatistics TemporalAccumulator::poolStatistics() const
{
    return _pool ? _pool->statistics() : TemporaryPoolStatistics{};
}

}
