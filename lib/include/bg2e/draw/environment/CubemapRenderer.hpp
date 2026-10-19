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
#include <bg2e/gpu/CubeMap.hpp>
#include <bg2e/gpu/GraphicsPipeline.hpp>
#include <bg2e/gpu/ResourceSet.hpp>

#include <filesystem>
#include <bg2e/base/Color.hpp>

namespace bg2e::draw::environment {
enum class GenerationKind
{
    Sphere, ProceduralSky, Irradiance, Specular, BRDF
};
struct GenerationSettings {
    uint32_t size = 256;
    uint32_t mipLevels = 1; // Zero: complete chain. Irradiance/BRDF require one.
    uint32_t samples = 256;
    gpu::PixelFormat format = gpu::PixelFormat::R16G16B16A16_SFLOAT;
    std::filesystem::path shaderRoot;
    std::string vertexShader = "draw/environment/fullscreen";
    std::string fragmentShader; // Empty selects the generation kind's shader.

    // Linear RGB colors for ProceduralSky; alpha is ignored.
    base::Color skyHorizonColor{0.55f, 0.65f, 0.8f};
    base::Color skyZenithColor{0.06f, 0.22f, 0.6f};
    base::Color groundColor{0.04f, 0.035f, 0.025f};
};

// Public GPU-only generation service and EnvironmentPass implementation.
// Settings are snapshotted by initialize/generate. Stop producers before source,
// lifecycle or output changes. record targets owned resources; no external targets.
class BG2E_API CubemapRenderer : public EnvironmentPass {
  public:
    explicit CubemapRenderer(GenerationKind kind, GenerationSettings settings = {});
    CubemapRenderer(const CubemapRenderer&) = delete;
    CubemapRenderer& operator=(const CubemapRenderer&) = delete;
    GenerationSettings& settings()
    {
        return _settings;
    }
    const GenerationSettings& settings() const
    {
        return _settings;
    }
    void setSource(std::shared_ptr<gpu::Image> source);
    virtual std::shared_ptr<gpu::Image> generate(Engine& engine, std::shared_ptr<gpu::Image> source = nullptr);
    const EnvironmentResources& resources() const override;
    const pipeline::PassContract& contract() const override;
    void initialize(Engine& engine, const pipeline::LayerConfiguration& configuration) override;
    void attachScene(std::shared_ptr<scene::Node>) override;
    void resize(const pipeline::LayerConfiguration& configuration) override;
    void update(const FrameContext&) override;
    void record(const pipeline::PassContext& context) override;
    void cleanup() override;
    std::shared_ptr<gpu::Image> image() const;

  protected:
    virtual void recordGeneration(gpu::CommandBuffer& command);
    GenerationKind _kind;
    GenerationSettings _settings;
    GenerationSettings _realized;
    Engine* _engine = nullptr;
    std::shared_ptr<gpu::Image> _source;
    std::shared_ptr<gpu::Image> _output;
    std::shared_ptr<gpu::CubeMap> _cube;
    std::shared_ptr<gpu::Sampler> _sourceSampler;
    std::shared_ptr<gpu::ShaderModule> _vertex;
    std::shared_ptr<gpu::ShaderModule> _fragment;
    std::shared_ptr<gpu::PipelineLayout> _layout;
    std::shared_ptr<gpu::GraphicsPipeline> _pipeline;
    std::shared_ptr<gpu::ResourceSet> _sourceSet;
    EnvironmentResources _resources;
    mutable pipeline::PassContract _contract;
};

class BG2E_API ProceduralSkyRenderer : public CubemapRenderer {
  public:
    ProceduralSkyRenderer();
};
}
