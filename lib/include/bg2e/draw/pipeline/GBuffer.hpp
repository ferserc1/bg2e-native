/*
 *    business grade graphic engine (bg2 engine)
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
    std::shared_ptr<gpu::GraphicsPipeline> _pipeline, _transparentPipeline;
    std::shared_ptr<gpu::ShaderModule> _vertex, _fragment;
};
}
