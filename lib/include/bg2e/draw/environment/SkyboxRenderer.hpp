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

#include <bg2e/draw/environment/EnvironmentResources.hpp>
#include <bg2e/draw/Engine.hpp>
#include <bg2e/gpu/GraphicsPipeline.hpp>
#include <bg2e/gpu/ResourceSet.hpp>
#include <bg2e/math/base.hpp>

#include <filesystem>

namespace bg2e::draw::environment {
struct SkyboxSettings {
    std::filesystem::path shaderRoot;
    std::string vertexShader = "draw/environment/fullscreen";
    std::string fragmentShader = "draw/environment/skybox";
    float exposure = 0.0f; // Linear HDR multiplier exp2(exposure), no tone mapping.
};
class BG2E_API SkyboxRenderer : public SkyboxPass {
  public:
    SkyboxSettings& settings()
    {
        return _settings;
    }
    float exposure() const;
    void setExposure(float value);
    void setCamera(const glm::mat4& view, const glm::mat4& projection) override;
    void setEnvironment(std::shared_ptr<const EnvironmentResources> resources) override;
    // Standalone draw records inside an existing compatible rendering scope.
    void initialize(Engine& engine, gpu::PixelFormat color, gpu::PixelFormat depth);
    void draw(gpu::CommandBuffer& command, gpu::Size2D extent);
    const pipeline::PassContract& contract() const override;
    void initialize(Engine& engine, const pipeline::LayerConfiguration& configuration) override;
    void attachScene(std::shared_ptr<scene::Node>) override;
    void resize(const pipeline::LayerConfiguration& configuration) override;
    void update(const FrameContext&) override;
    // Layer form loads color/depth outputs, draws, and restores sampled color.
    void record(const pipeline::PassContext& context) override;
    void cleanup() override;
  private:
    SkyboxSettings _settings;
    Engine* _engine = nullptr;
    glm::mat4 _inverseViewProjection{1};
    std::shared_ptr<const EnvironmentResources> _environment;
    std::shared_ptr<gpu::ShaderModule> _vertex;
    std::shared_ptr<gpu::ShaderModule> _fragment;
    std::shared_ptr<gpu::PipelineLayout> _layout;
    std::shared_ptr<gpu::GraphicsPipeline> _pipeline;
    std::shared_ptr<gpu::ResourceSet> _skySet;
    std::shared_ptr<gpu::ResourceSet> _blurSet;
    pipeline::PassContract _contract{"draw.environment.skybox.v1", {}, {}};
};
}
