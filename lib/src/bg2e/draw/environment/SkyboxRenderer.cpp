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

#include <bg2e/draw/environment/SkyboxRenderer.hpp>
#include <bg2e/gpu/Device.hpp>
#include <bg2e/gpu/ShaderLib.hpp>

#include <cmath>
#include <stdexcept>
#include <utility>

namespace bg2e::draw::environment {
namespace {
struct SkyboxPush {
    glm::mat4 inverseViewProjection;
    glm::vec4 parameters; // width, height, specular LOD, exposure
};
static_assert(sizeof(SkyboxPush) == 80);
}
float SkyboxRenderer::exposure() const
{
    return _settings.exposure;
}
void SkyboxRenderer::setExposure(float value)
{
    if (!std::isfinite(value) || std::abs(value) > 32)
    {
        throw std::invalid_argument("Skybox exposure must be finite and in [-32,32]");
    }
    _settings.exposure = value;
}
void SkyboxRenderer::setCamera(const glm::mat4& view, const glm::mat4& projection)
{
    const auto matrix = projection * glm::mat4(glm::mat3(view));
    if (!std::isfinite(glm::determinant(matrix)) || std::abs(glm::determinant(matrix)) < 1e-12f)
    {
        throw std::invalid_argument("Skybox camera must have a finite invertible rotation/projection");
    }
    _inverseViewProjection = glm::inverse(matrix);
}
void SkyboxRenderer::setEnvironment(std::shared_ptr<const EnvironmentResources> resources)
{
    if (_engine)
    {
        _engine->device()->waitIdleForResourceUpdate();
    }
    if (resources && _engine && _layout && _pipeline)
    {
        resources->validate(_engine->device());
        auto skySet = _engine->device()->createResourceSet(_layout.get(), 0);
        skySet->setSampledImage({.vulkan = 0, .metal = 0}, resources->sky.get());
        skySet->setSampler({.vulkan = 1, .metal = 0}, resources->sampler.get());
        skySet->update();
        auto blurSet = _engine->device()->createResourceSet(_layout.get(), 0);
        blurSet->setSampledImage({.vulkan = 0, .metal = 0}, resources->prefilteredSpecular.get());
        blurSet->setSampler({.vulkan = 1, .metal = 0}, resources->sampler.get());
        blurSet->update();
        _skySet = std::move(skySet);
        _blurSet = std::move(blurSet);
    }
    else
    {
        _skySet.reset();
        _blurSet.reset();
    }
    _environment = std::move(resources);
}
void SkyboxRenderer::initialize(Engine& engine, gpu::PixelFormat color, gpu::PixelFormat depth)
{
    if (_engine && _engine != &engine)
    {
        throw std::logic_error("Clean up skybox before changing engines");
    }
    auto environment = _environment;
    cleanup();
    _engine = &engine;
    gpu::PipelineLayoutDescription layout;
    layout.pushConstants.push_back({0, sizeof(SkyboxPush), gpu::ShaderStage::Fragment});
    layout.resourceBindings.push_back({0, {.vulkan = 0, .metal = 0}, gpu::ResourceType::SampledImage, gpu::ShaderStage::Fragment, 1});
    layout.resourceBindings.push_back({0, {.vulkan = 1, .metal = 0}, gpu::ResourceType::Sampler, gpu::ShaderStage::Fragment, 1});
    _layout = engine.device()->createPipelineLayout(layout);
    gpu::ShaderLib library(_settings.shaderRoot, engine.backendType());
    _vertex = library.vertex(_settings.vertexShader, engine.device());
    _fragment = library.fragment(_settings.fragmentShader, engine.device());
    gpu::GraphicsPipelineDescription description;
    description.vertexShader = _vertex.get();
    description.fragmentShader = _fragment.get();
    description.layout = _layout.get();
    description.colorFormat = color;
    description.depthFormat = depth;
    description.cullMode = gpu::CullMode::None;
    description.depthState = gpu::DepthState{depth != gpu::PixelFormat::Undefined, false, gpu::CompareOperation::LessOrEqual};
    _pipeline = engine.device()->createGraphicsPipeline(description);
    _contract.outputs = {{"color", color, pipeline::ExtentDomain::Internal, gpu::ImageUsage::ColorAttachment | gpu::ImageUsage::Sampled}};
    if (depth != gpu::PixelFormat::Undefined)
    {
        _contract.outputs.push_back({"depth", depth, pipeline::ExtentDomain::Internal, gpu::ImageUsage::DepthStencil});
    }
    setEnvironment(std::move(environment));
}
void SkyboxRenderer::initialize(Engine& engine, const pipeline::LayerConfiguration& configuration)
{
    gpu::PixelFormat color = gpu::PixelFormat::Undefined;
    gpu::PixelFormat depth = gpu::PixelFormat::Undefined;
    for (const auto& output : configuration.contract.outputs)
    {
        if (output.semantic == "color")
        {
            color = output.format;
        }
        else if (output.semantic == "depth")
        {
            depth = output.format;
        }
    }
    if (color == gpu::PixelFormat::Undefined)
    {
        throw std::invalid_argument("Skybox layer configuration requires a color output format");
    }
    initialize(engine, color, depth);
}
void SkyboxRenderer::draw(gpu::CommandBuffer& command, gpu::Size2D extent)
{
    if (!_pipeline || !_environment || !_skySet || !_blurSet || extent.width == 0 || extent.height == 0 ||
        !std::isfinite(_settings.exposure) || std::abs(_settings.exposure) > 32.0f)
    {
        throw std::logic_error("Skybox draw requires initialized resources, a valid extent and finite exposure in [-32, 32]");
    }
    if (!_environment->skyboxVisible)
    {
        return;
    }
    const auto blur = _environment->skyboxBlur;
    SkyboxPush push{_inverseViewProjection, {float(extent.width), float(extent.height),
        blur * float(_environment->prefilteredSpecular->mipLevels() - 1), _settings.exposure}};
    command.bindPipeline(_pipeline.get());
    command.bindResourceSet(_pipeline.get(), 0, blur > 0 ? _blurSet.get() : _skySet.get());
    command.pushConstants(gpu::ShaderStage::Fragment, 0, sizeof(push), &push);
    command.draw(3);
}
void SkyboxRenderer::record(const pipeline::PassContext& context)
{
    if (&context.frame.engine != _engine)
    {
        throw std::invalid_argument("Skybox belongs to another engine");
    }
    if (!_pipeline || !_environment || !_skySet || !_blurSet || !context.inputs.empty() ||
        !std::isfinite(_settings.exposure) || std::abs(_settings.exposure) > 32.0f)
    {
        throw std::logic_error("Skybox resources or settings are not ready");
    }
    _environment->validate(_engine->device());
    if (!_environment->skyboxVisible)
    {
        return;
    }
    gpu::Image* color = nullptr;
    gpu::Image* depth = nullptr;
    for (const auto& output : context.outputs)
    {
        if (output.semantic == "color")
        {
            color = output.image;
        }
        else if (output.semantic == "depth")
        {
            depth = output.image;
        }
    }
    if (context.outputs.size() != _contract.outputs.size() || !color || !color->isValid() ||
        !gpu::hasFlag(color->usage(), gpu::ImageUsage::ColorAttachment) || !gpu::hasFlag(color->usage(), gpu::ImageUsage::Sampled) ||
        color->isCubemap() || (depth && (!depth->isValid() || depth->isCubemap() || !gpu::hasFlag(depth->usage(), gpu::ImageUsage::DepthStencil))) || color->device() != _engine->device() || color->pixelFormat() != _contract.outputs.front().format ||
        (depth != nullptr) != (_pipeline->depthFormat() != gpu::PixelFormat::Undefined) ||
        (depth && (depth->device() != _engine->device() || depth->pixelFormat() != _pipeline->depthFormat() ||
            depth->width() != color->width() || depth->height() != color->height())))
    {
        throw std::invalid_argument("Skybox output attachments do not match its pipeline");
    }
    if (color->currentLayout({0, 1, 0, 1}) == gpu::ImageLayout::Undefined ||
        (depth && depth->currentLayout({0, 1, 0, 1}) == gpu::ImageLayout::Undefined))
    {
        throw std::invalid_argument("Skybox loads existing initialized color/depth contents");
    }
    auto& command = context.frame.commandBuffer;
    command.transition(color, gpu::ImageLayout::ColorAttachment, {0, 1, 0, 1});
    gpu::RenderingDescription rendering;
    rendering.colors.push_back({.image = color});
    if (depth)
    {
        command.transition(depth, gpu::ImageLayout::DepthAttachment, {0, 1, 0, 1});
        rendering.depth = gpu::DepthAttachmentDescription{.image = depth};
    }
    command.beginRendering(rendering);
    draw(command, color->size());
    command.endRendering();
    command.transition(color, gpu::ImageLayout::ShaderReadOnly, {0, 1, 0, 1});
}
const pipeline::PassContract& SkyboxRenderer::contract() const
{
    return _contract;
}
void SkyboxRenderer::attachScene(std::shared_ptr<scene::Node>)
{
}
void SkyboxRenderer::update(const FrameContext&)
{
}
void SkyboxRenderer::resize(const pipeline::LayerConfiguration& configuration)
{
    if (!_engine)
    {
        throw std::logic_error("Initialize skybox before resize");
    }
    initialize(*_engine, configuration);
}
void SkyboxRenderer::cleanup()
{
    if (_engine)
    {
        _engine->device()->waitIdleForResourceUpdate();
    }
    _skySet.reset();
    _blurSet.reset();
    _pipeline.reset();
    _layout.reset();
    _vertex.reset();
    _fragment.reset();
    _environment.reset();
    _engine = nullptr;
}
}
