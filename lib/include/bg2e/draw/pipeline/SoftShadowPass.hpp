#pragma once

#include <bg2e/draw/pipeline/RenderLayer.hpp>
#include <bg2e/draw/CollectRayTracingInstancesVisitor.hpp>
#include <bg2e/gpu/ComputePipeline.hpp>
#include <bg2e/gpu/Buffer.hpp>
#include <bg2e/gpu/ResourceSet.hpp>
#include <bg2e/gpu/Sampler.hpp>
#include <filesystem>

namespace bg2e::draw::pipeline {
inline constexpr uint32_t MaxShadowAlphaTextures = 8;
inline constexpr uint32_t MaxShadowSamples = 64;
struct SoftShadowSettings {
    bool enabled = true;
    float bias = 0.01f;
    float directionalDistance = 1000000.0f;
    // Per-light shadowSamples is capped to this explicit quality budget.
    uint32_t maxSamples = 16;
};
// Dense TLAS-instance order; ranges.w retains the original frame object ID.
// UV triples are resolved from the actual submesh firstIndex, not global IDs.
struct alignas(16) ShadowInstanceData {
    glm::uvec4 ranges{0}; // first UV triangle, count, alpha slot or UINT_MAX, objectId
    glm::vec4 alpha{1}; // scale.xy, cutoff, albedo alpha multiplier
    glm::uvec4 options{0}; // albedo UV set, material flags, reserved, reserved
};
struct alignas(16) ShadowTriangleUV {
    glm::vec4 a{0}, b{0}, c{0}; // UV0.xy, UV1.zw for each indexed vertex
};
struct ShadowQueryTable {
    std::vector<ShadowInstanceData> instances;
    std::vector<ShadowTriangleUV> triangles;
    std::vector<std::shared_ptr<Texture>> alphaTextures;
};
BG2E_API ShadowQueryTable buildShadowQueryTable(const CollectRayTracingInstancesVisitor& collection);
BG2E_API PassContract softShadowContract(gpu::PixelFormat depth);
BG2E_API gpu::PipelineLayoutDescription softShadowBindingLayout();
static_assert(sizeof(ShadowInstanceData) == 48 && sizeof(ShadowTriangleUV) == 48);

// Public compute-based visibility pass. No render-stage query capability or RT
// pipeline/SBT is needed. Unsupported creation reports status and allocates no TLAS.
class BG2E_API SoftShadowPass : public RenderLayer {
  public:
    explicit SoftShadowPass(std::filesystem::path shaderRoot = {}, uint32_t slots = 0,
        gpu::PixelFormat depth = gpu::PixelFormat::D32_SFLOAT);
    SoftShadowSettings& settings();
    const SoftShadowSettings& settings() const;
    bool available() const;
    const std::string& status() const;
    const PassContract& contract() const override;
    void initialize(Engine& engine, const LayerConfiguration& configuration) override;
    void attachScene(std::shared_ptr<scene::Node>) override;
    void resize(const LayerConfiguration& configuration) override;
    void update(const FrameContext&) override;
    // After acquiring the completed slot, once per presentation, before consumers.
    void prepare(const FrameContext& frame, const RenderQueue& queue,
        const glm::mat4& view, const glm::mat4& projection);
    void record(const PassContext& context) override;
    void cleanup() override;
  protected:
    struct Slot {
        CollectRayTracingInstancesVisitor collection;
        ShadowQueryTable table;
        std::shared_ptr<gpu::RayTracingScene> scene;
        std::shared_ptr<gpu::Buffer> uniforms, instances, triangles;
        std::vector<std::shared_ptr<gpu::ResourceSet>> sets;
        uint64_t serial = 0;
        bool prepared = false;
        bool active = false;
        bool built = false;
    };
    std::filesystem::path _shaderRoot;
    uint32_t _requestedSlots;
    SoftShadowSettings _settings;
    Engine* _engine = nullptr;
    PassContract _contract;
    std::string _status = "Not initialized";
    std::shared_ptr<gpu::ShaderModule> _shader;
    std::shared_ptr<gpu::PipelineLayout> _layout;
    std::shared_ptr<gpu::ComputePipeline> _pipeline;
    std::shared_ptr<gpu::Sampler> _nearest;
    std::vector<Slot> _slots;
};
}
