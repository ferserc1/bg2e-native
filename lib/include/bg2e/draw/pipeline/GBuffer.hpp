#pragma once

#include <bg2e/draw/pipeline/RenderLayer.hpp>
#include <bg2e/draw/pipeline/RasterFrame.hpp>
#include <bg2e/gpu/GraphicsPipeline.hpp>
#include <filesystem>

namespace bg2e::draw::pipeline {
BG2E_API PassContract gbufferContract(gpu::PixelFormat depth);
BG2E_API std::vector<gpu::PixelFormat> gbufferFormats();
class BG2E_API GBuffer : public RenderLayer {
  public:
    explicit GBuffer(std::filesystem::path shaderRoot = {}, gpu::PixelFormat depth = gpu::PixelFormat::D32_SFLOAT);
    const PassContract& contract() const override;
    void initialize(Engine& engine, const LayerConfiguration& configuration) override;
    void attachScene(std::shared_ptr<scene::Node>) override;
    void resize(const LayerConfiguration& configuration) override;
    void update(const FrameContext&) override;
    void record(const PassContext& context) override;
    void cleanup() override;
  protected:
    std::filesystem::path _shaderRoot;
    Engine* _engine = nullptr;
    PassContract _contract;
    std::shared_ptr<gpu::PipelineLayout> _layout;
    std::shared_ptr<gpu::GraphicsPipeline> _pipeline;
    std::shared_ptr<gpu::ShaderModule> _vertex, _fragment;
};
}
