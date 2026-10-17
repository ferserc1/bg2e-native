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
#include <bg2e/draw/pipeline/RTAmbientOcclusion.hpp>
#include <bg2e/reflection/TypeInfo.hpp>
#include <set>
#include <tuple>

namespace bg2e::draw::pipeline {
enum class DenoiseSignal : uint32_t
{
    Visibility, DiffuseRadiance, ReflectedRadiance
};
struct DenoiseKey {
    DenoiseSignal signal = DenoiseSignal::Visibility;
    uint64_t view = 0, layer = 0;

    bool operator<(const DenoiseKey& other) const
    {
        return std::tie(signal,view,layer) < std::tie(other.signal,other.view,other.layer);
    }
};
struct DenoiseSettings {
    bool enabled = true;
    uint32_t kernelRadius = 2;
    float depthThreshold = 0.1f, normalThreshold = 0.8f;
    float depthSigma = 0.1f, normalSigma = 0.3f, spatialSigma = 2;
};
struct DenoiseConfiguration {
    std::filesystem::path shaderRoot;
    std::string shaderPrefix = "draw/denoise", shaderInterface = "draw.denoise.v1";
    uint32_t maxInvocations = 256; // Per acquired slot, across effect/view/layer keys.
};
struct DenoiseInput {
    gpu::CommandBuffer& command;
    uint32_t slot;
    uint64_t serial;
    DenoiseKey key;
    gpu::Image& signal;
    SurfaceInputs surface;
    glm::mat4 worldToView{1}; // Camera profile: linear view-space Z comparison.
    gpu::Image* material = nullptr; // Reflection roughness G at surface extent.
    float maxRoughness = 1;
};
struct DenoiseResult {
    gpu::Image* image = nullptr;
    std::shared_ptr<gpu::Image> owner; // Empty for disabled/radius-zero/unsupported pass-through.
};
// Stateless spatial filtering after temporal accumulation, before composition.
// Caller retains borrowed input images through completion. Scratch is per-call,
// retained in a completed-slot lifecycle, and never used as temporal history.
class BG2E_API DenoiseFilter {
  public:
    explicit DenoiseFilter(DenoiseConfiguration configuration = {});
    virtual ~DenoiseFilter() = default;
    DenoiseFilter(const DenoiseFilter&) = delete;
    DenoiseFilter& operator=(const DenoiseFilter&) = delete;

    virtual void initialize(gpu::Device& device, gpu::BackendType backend, uint32_t slots);
    virtual void beginFrame(uint32_t slot, uint64_t serial);
    virtual DenoiseResult record(const DenoiseInput& input);
    virtual void cleanup();

    virtual bool enabled() const;
    virtual void setEnabled(bool value);
    virtual uint32_t kernelRadius() const;
    virtual void setKernelRadius(uint32_t value);
    virtual float depthThreshold() const;
    virtual void setDepthThreshold(float value);
    virtual float normalThreshold() const;
    virtual void setNormalThreshold(float value);
    virtual float depthSigma() const;
    virtual void setDepthSigma(float value);
    virtual float normalSigma() const;
    virtual void setNormalSigma(float value);
    virtual float spatialSigma() const;
    virtual void setSpatialSigma(float value);
    virtual bool available() const;
    virtual bool available(DenoiseSignal signal, SurfaceProfile surface) const;
    const std::string& status() const;
    virtual reflection::TypeInfo settingsType() const;
    virtual void* settingsInstance();

  protected:
    struct Invocation {
        std::shared_ptr<gpu::Image> output;
        std::shared_ptr<gpu::Buffer> uniforms;
        std::shared_ptr<gpu::ResourceSet> resources;
    };
    struct Slot {
        uint64_t serial = 0;
        bool prepared = false;
        std::set<DenoiseKey> keys;
        std::vector<Invocation> invocations;
    };
    DenoiseConfiguration _configuration;
    DenoiseSettings _settings;
    gpu::Device* _device = nullptr;
    std::shared_ptr<gpu::PipelineLayout> _layout;
    std::shared_ptr<gpu::Sampler> _nearest;
    std::array<std::shared_ptr<gpu::ShaderModule>,4> _shaders;
    std::array<std::shared_ptr<gpu::ComputePipeline>,4> _pipelines;
    std::vector<Slot> _slots;
    std::string _status = "Not initialized";
};
}
