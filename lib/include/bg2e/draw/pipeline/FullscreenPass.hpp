#pragma once

#include <bg2e/draw/pipeline/RenderLayer.hpp>
#include <bg2e/gpu/GraphicsPipeline.hpp>
#include <bg2e/gpu/ResourceSet.hpp>
#include <bg2e/gpu/Sampler.hpp>
#include <filesystem>
#include <utility>

namespace bg2e::draw::pipeline {
// Reusable procedural triangle; explicit layout/shaders and per-call resource
// sets. beginFrame releases only the acquired, completed slot's previous sets.
class BG2E_API FullscreenPass {
  public:
    void initialize(Engine& engine, const std::filesystem::path& shaderRoot,
        const std::string& fragment, std::shared_ptr<gpu::PipelineLayout> layout,
        gpu::PixelFormat output, uint32_t slots);
    void beginFrame(const FrameContext& frame);
    void draw(const FrameContext& frame, gpu::Image& output,
        const std::vector<gpu::Image*>& inputs, uint32_t imageSet,
        const std::vector<std::pair<uint32_t, gpu::ResourceSet*>>& sets,
        const void* push = nullptr, uint32_t pushSize = 0, const std::vector<uint32_t>& metalImages = {});
    void cleanup();
  private:
    Engine* _engine = nullptr;
    std::shared_ptr<gpu::PipelineLayout> _layout;
    std::shared_ptr<gpu::ShaderModule> _vertex, _fragment;
    std::shared_ptr<gpu::GraphicsPipeline> _pipeline;
    std::shared_ptr<gpu::Sampler> _sampler;
    std::vector<std::vector<std::shared_ptr<gpu::ResourceSet>>> _sets;
    std::vector<uint64_t> _serial;
};
}
