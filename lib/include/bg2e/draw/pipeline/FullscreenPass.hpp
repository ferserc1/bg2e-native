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
#include <bg2e/draw/pipeline/TemporaryResourcePool.hpp>

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
        const void* push = nullptr, uint32_t pushSize = 0, const std::vector<uint32_t>& metalImages = {},
        const std::vector<uint32_t>& metalSamplers = {},
        std::optional<gpu::ScissorRegion> region = std::nullopt);
    void cleanup();

    TemporaryPoolStatistics poolStatistics() const;

    // Diagnostic baseline: allocate fresh invocation/image storage.
    void setPoolingEnabled(bool enabled)
    {
        _poolingEnabled = enabled;
    }

  private:
    Engine* _engine = nullptr;
    std::shared_ptr<TemporaryResourcePool> _pool;
    bool _poolingEnabled = true;
    std::shared_ptr<gpu::PipelineLayout> _layout;
    std::shared_ptr<gpu::ShaderModule> _vertex, _fragment;
    std::shared_ptr<gpu::GraphicsPipeline> _pipeline;
    std::shared_ptr<gpu::Sampler> _sampler;
    std::vector<std::vector<std::shared_ptr<TemporaryBindings>>> _sets;
    std::vector<uint64_t> _serial;
};
}
