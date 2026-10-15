#include <bg2e/draw/environment/CubemapRenderer.hpp>
#include <bg2e/draw/environment/SphereToCubemapRenderer.hpp>
#include <bg2e/draw/environment/IrradianceCubemapRenderer.hpp>
#include <bg2e/draw/environment/SpecularReflectionCubemapRenderer.hpp>
#include <bg2e/draw/environment/BRDFIntegrationMapTextureGenerator.hpp>
#include <bg2e/gpu/CubemapRenderPass.hpp>
#include <bg2e/gpu/Device.hpp>
#include <bg2e/gpu/ShaderLib.hpp>
#include <bg2e/gpu/Sampler.hpp>

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace bg2e::draw::environment {
namespace {
struct GenerationPush {
    uint32_t face;
    uint32_t size;
    uint32_t samples;
    uint32_t sourceSize;
    float roughness;
    float sourceMaxLod;
    float padding[2] = {};
};
static_assert(sizeof(GenerationPush) == 32);
std::string semantic(GenerationKind kind)
{
    switch (kind)
    {
    case GenerationKind::Sphere: return "sky";
    case GenerationKind::ProceduralSky: return "sky";
    case GenerationKind::Irradiance: return "irradiance";
    case GenerationKind::Specular: return "prefilteredSpecular";
    case GenerationKind::BRDF: return "brdf";
    }
    throw std::invalid_argument("Unknown environment generation kind");
}
std::string shader(GenerationKind kind)
{
    switch (kind)
    {
    case GenerationKind::Sphere: return "sphere";
    case GenerationKind::ProceduralSky: return "procedural_sky";
    case GenerationKind::Irradiance: return "irradiance";
    case GenerationKind::Specular: return "prefilter";
    case GenerationKind::BRDF: return "brdf";
    }
    throw std::invalid_argument("Unknown environment generation kind");
}
}
CubemapRenderer::CubemapRenderer(GenerationKind kind, GenerationSettings settings)
    : _kind(kind), _settings(std::move(settings))
{
    _contract.shaderInterface = "draw.environment." + semantic(kind) + ".v1";
    _contract.outputs = {{semantic(kind), _settings.format, pipeline::ExtentDomain::Internal,
        gpu::ImageUsage::Sampled | gpu::ImageUsage::ColorAttachment | gpu::ImageUsage::TransferSrc,
        kind == GenerationKind::BRDF ? gpu::ImageType::Image2D : gpu::ImageType::Cubemap}};
}
SphereToCubemapRenderer::SphereToCubemapRenderer() : CubemapRenderer(GenerationKind::Sphere)
{
    _settings.mipLevels = 0;
}
ProceduralSkyRenderer::ProceduralSkyRenderer() : CubemapRenderer(GenerationKind::ProceduralSky)
{
    _settings.mipLevels = 0;
}
IrradianceCubemapRenderer::IrradianceCubemapRenderer() : CubemapRenderer(GenerationKind::Irradiance)
{
    _settings.size = 32;
}
SpecularReflectionCubemapRenderer::SpecularReflectionCubemapRenderer() : CubemapRenderer(GenerationKind::Specular)
{
    _settings.mipLevels = 0;
}
BRDFIntegrationMapTextureGenerator::BRDFIntegrationMapTextureGenerator() : CubemapRenderer(GenerationKind::BRDF)
{
    _settings.size = 128;
}
void CubemapRenderer::setSource(std::shared_ptr<gpu::Image> source)
{
    if (_engine || _pipeline)
    {
        throw std::logic_error("Clean up environment pass before changing its source");
    }
    _source = std::move(source);
}
std::shared_ptr<gpu::Image> CubemapRenderer::generate(Engine& engine, std::shared_ptr<gpu::Image> source)
{
    cleanup();
    setSource(std::move(source));
    initialize(engine, {});
    engine.device()->immediateSubmit([this](gpu::CommandBuffer* command)
    {
        recordGeneration(*command);
    });
    return _output;
}
void CubemapRenderer::initialize(Engine& engine, const pipeline::LayerConfiguration& configuration)
{
    if (_engine && _engine != &engine)
    {
        throw std::logic_error("Clean up environment pass before changing engines");
    }
    auto source = _source;
    cleanup();
    _source = std::move(source);
    _realized = _settings;
    const bool hasSource = _kind == GenerationKind::Sphere || _kind == GenerationKind::Irradiance || _kind == GenerationKind::Specular;
    auto* device = engine.device();
    if (_realized.size == 0 || _realized.size > device->renderingCapabilities().maxImageDimension2D ||
        _realized.samples == 0 || _realized.samples > 4096 ||
        (_realized.format != gpu::PixelFormat::R16G16B16A16_SFLOAT && _realized.format != gpu::PixelFormat::R32G32B32A32_SFLOAT))
    {
        throw std::invalid_argument("Invalid environment extent, sample count or linear HDR format");
    }
    const auto levels = _realized.mipLevels == 0 ? gpu::mipLevelsForMinSize(_realized.size, 1) : _realized.mipLevels;
    if (levels > gpu::mipLevelsForMinSize(_realized.size, 1) ||
        ((_kind == GenerationKind::BRDF || _kind == GenerationKind::Irradiance) && levels != 1) ||
        (_kind == GenerationKind::Specular && levels < 2))
    {
        throw std::invalid_argument("Invalid environment mip count");
    }
    const auto usage = gpu::ImageUsage::Sampled | gpu::ImageUsage::ColorAttachment | gpu::ImageUsage::TransferSrc;
    if (!device->formatCapabilities(_realized.format).supports(usage))
    {
        throw std::runtime_error("Environment target format does not support sampled HDR rendering/readback");
    }
    if ((!configuration.contract.shaderInterface.empty() && configuration.contract.shaderInterface != _contract.shaderInterface) ||
        !configuration.contract.inputs.empty() || configuration.contract.outputs.size() > 1 ||
        (!configuration.contract.outputs.empty() &&
            (configuration.contract.outputs.front().semantic != semantic(_kind) ||
             configuration.contract.outputs.front().format != _realized.format ||
             configuration.contract.outputs.front().type != (_kind == GenerationKind::BRDF ? gpu::ImageType::Image2D : gpu::ImageType::Cubemap))))
    {
        throw std::invalid_argument("Environment generation contract is incompatible with its owned output/settings");
    }
    if (hasSource && (!_source || !_source->isValid() || _source->device() != device ||
        !gpu::hasFlag(_source->usage(), gpu::ImageUsage::Sampled) ||
        _source->isCubemap() != (_kind != GenerationKind::Sphere) ||
        (_source->pixelFormat() != gpu::PixelFormat::R16G16B16A16_SFLOAT && _source->pixelFormat() != gpu::PixelFormat::R32G32B32A32_SFLOAT) ||
        _source->currentLayout(_source->fullRange()) != gpu::ImageLayout::ShaderReadOnly))
    {
        throw std::invalid_argument("Environment source must be a same-device shader-readable linear HDR image of the required type");
    }
    _engine = &engine;
    if (_kind == GenerationKind::BRDF)
    {
        _output = device->createImage({.size = {_realized.size, _realized.size}, .format = _realized.format,
            .usage = usage, .debugName = "Environment split-sum BRDF LUT"});
        _resources.brdf = _output;
    }
    else
    {
        _cube = device->createCubeMap({.size = _realized.size, .format = _realized.format,
            .usage = usage, .mipLevels = levels, .debugName = "Environment " + semantic(_kind)});
        _output = std::shared_ptr<gpu::Image>(_cube, _cube->image());
        if (_kind == GenerationKind::Irradiance)
        {
            _resources.irradiance = _output;
        }
        else if (_kind == GenerationKind::Specular)
        {
            _resources.prefilteredSpecular = _output;
        }
        else
        {
            _resources.sky = _output;
        }
    }
    _contract.outputs = {{semantic(_kind), _realized.format, pipeline::ExtentDomain::Internal, usage,
        _kind == GenerationKind::BRDF ? gpu::ImageType::Image2D : gpu::ImageType::Cubemap}};
    gpu::PipelineLayoutDescription layout;
    layout.pushConstants.push_back({0, sizeof(GenerationPush), gpu::ShaderStage::Fragment});
    if (hasSource)
    {
        layout.resourceBindings.push_back({0, {.vulkan = 0, .metal = 0}, gpu::ResourceType::SampledImage, gpu::ShaderStage::Fragment, 1});
        layout.resourceBindings.push_back({0, {.vulkan = 1, .metal = 0}, gpu::ResourceType::Sampler, gpu::ShaderStage::Fragment, 1});
    }
    _layout = device->createPipelineLayout(layout);
    gpu::ShaderLib library(_realized.shaderRoot, engine.backendType());
    _vertex = library.vertex(_realized.vertexShader, device);
    _fragment = library.fragment(_realized.fragmentShader.empty() ? "draw/environment/" + shader(_kind) : _realized.fragmentShader, device);
    gpu::GraphicsPipelineDescription pipeline;
    pipeline.vertexShader = _vertex.get();
    pipeline.fragmentShader = _fragment.get();
    pipeline.layout = _layout.get();
    pipeline.colorFormat = _realized.format;
    pipeline.cullMode = gpu::CullMode::None;
    pipeline.depthState = gpu::DepthState{false, false, gpu::CompareOperation::Always};
    _pipeline = device->createGraphicsPipeline(pipeline);
    if (hasSource)
    {
        const bool linear = device->formatCapabilities(_source->pixelFormat()).linearFiltering;
        gpu::SamplerDescription sampler;
        sampler.minFilter = sampler.magFilter = linear ? gpu::Filter::Linear : gpu::Filter::Nearest;
        sampler.mipFilter = linear ? gpu::MipmapFilter::Linear : gpu::MipmapFilter::Nearest;
        sampler.addressModeU = _source->isCubemap() ? gpu::AddressMode::ClampToEdge : gpu::AddressMode::Repeat;
        sampler.addressModeV = sampler.addressModeW = gpu::AddressMode::ClampToEdge;
        sampler.maxLod = float(_source->mipLevels() - 1);
        _sourceSampler = device->createSampler(sampler);
        _sourceSet = device->createResourceSet(_layout.get(), 0);
        _sourceSet->setSampledImage({.vulkan = 0, .metal = 0}, _source.get());
        _sourceSet->setSampler({.vulkan = 1, .metal = 0}, _sourceSampler.get());
        _sourceSet->update();
    }
}
void CubemapRenderer::recordGeneration(gpu::CommandBuffer& command)
{
    if (!_pipeline || !_output || !_output->isValid() ||
        (_source && (!_source->isValid() || _source->currentLayout(_source->fullRange()) != gpu::ImageLayout::ShaderReadOnly)))
    {
        throw std::logic_error("Environment pass or its source is not ready");
    }
    auto draw = [&](uint32_t face, uint32_t mip)
    {
        command.bindPipeline(_pipeline.get());
        if (_sourceSet)
        {
            command.bindResourceSet(_pipeline.get(), 0, _sourceSet.get());
        }
        GenerationPush push{face, std::max(1u, _realized.size >> mip), _realized.samples,
            _source ? _source->width() : 1u,
            _kind == GenerationKind::Specular && _output->mipLevels() > 1 ? float(mip) / float(_output->mipLevels() - 1) : float(mip),
            _source ? float(_source->mipLevels() - 1) : 0.0f};
        command.pushConstants(gpu::ShaderStage::Fragment, 0, sizeof(push), &push);
        command.draw(3);
    };
    if (_cube)
    {
        gpu::CubemapRenderPass::render(&command, _cube.get(), {},
            [&](gpu::CommandBuffer*, gpu::CubemapFace face, uint32_t mip, const glm::mat4&, const glm::mat4&)
        {
            draw(uint32_t(face), mip);
        });
    }
    else
    {
        command.transition(_output.get(), gpu::ImageLayout::ColorAttachment);
        command.beginRendering(_output.get());
        draw(0, 0);
        command.endRendering();
        command.transition(_output.get(), gpu::ImageLayout::ShaderReadOnly);
    }
}
void CubemapRenderer::record(const pipeline::PassContext& context)
{
    if (&context.frame.engine != _engine || !context.inputs.empty() || context.outputs.size() > 1 ||
        (!context.outputs.empty() && (context.outputs.front().semantic != semantic(_kind) || context.outputs.front().image != _output.get())))
    {
        throw std::invalid_argument("Environment generation requires its own engine and internally owned bindings");
    }
    recordGeneration(context.frame.commandBuffer);
}
void CubemapRenderer::attachScene(std::shared_ptr<scene::Node>)
{
}
void CubemapRenderer::update(const FrameContext&)
{
}
void CubemapRenderer::resize(const pipeline::LayerConfiguration& configuration)
{
    if (!_engine)
    {
        throw std::logic_error("Initialize environment pass before resize");
    }
    initialize(*_engine, configuration);
}
const pipeline::PassContract& CubemapRenderer::contract() const
{
    if (!_pipeline && !_contract.outputs.empty())
    {
        _contract.outputs.front().format = _settings.format;
    }
    return _contract;
}
const EnvironmentResources& CubemapRenderer::resources() const
{
    return _resources;
}
std::shared_ptr<gpu::Image> CubemapRenderer::image() const
{
    return _output;
}
void CubemapRenderer::cleanup()
{
    if (_engine)
    {
        _engine->device()->waitIdle();
    }
    _sourceSet.reset();
    _pipeline.reset();
    _layout.reset();
    _vertex.reset();
    _fragment.reset();
    _sourceSampler.reset();
    _source.reset();
    _resources = {};
    _output.reset();
    _cube.reset();
    _engine = nullptr;
}
}
