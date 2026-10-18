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

#include <bg2e/bake/UvSurfacePass.hpp>
#include <bg2e/draw/Engine.hpp>
#include <bg2e/base/PlatformTools.hpp>
#include <bg2e/gpu/ShaderLib.hpp>
#include <cmath>
#include <cstring>
#include <limits>
#include <stdexcept>

namespace bg2e::bake {
namespace {
const std::array<gpu::PixelFormat,5> Formats{
    gpu::PixelFormat::R32G32B32A32_SFLOAT,gpu::PixelFormat::R16G16B16A16_SFLOAT,
    gpu::PixelFormat::R32_UINT,gpu::PixelFormat::R32_UINT,gpu::PixelFormat::R8_UNORM};
struct alignas(16) SurfaceUniforms {
    glm::mat4 model{1}, normal{1};
};
static_assert(sizeof(SurfaceUniforms) == 128);

SurfaceUniforms transform(const glm::mat4& model)
{
    for (uint32_t col = 0; col < 4; ++col)
    {
        for (uint32_t row = 0; row < 4; ++row)
        {
            if (!std::isfinite(model[col][row]))
            {
                throw std::invalid_argument("UV surface transform must be finite");
            }
        }
    }
    const float determinant = glm::determinant(glm::mat3(model));
    if (!std::isfinite(determinant) || std::abs(determinant) < 1e-12f ||
        model[0][3] != 0 || model[1][3] != 0 || model[2][3] != 0 || model[3][3] != 1)
    {
        throw std::invalid_argument("UV surface needs an invertible affine world transform");
    }
    SurfaceUniforms result{model,glm::transpose(glm::inverse(model))};
    for (uint32_t col = 0; col < 4; ++col)
    {
        for (uint32_t row = 0; row < 4; ++row)
        {
            if (!std::isfinite(result.normal[col][row]))
            {
                throw std::invalid_argument("UV surface normal transform overflow");
            }
        }
    }
    return result;
}

float halfFloat(uint16_t bits)
{
    const uint32_t exponent = (bits>>10)&31u, mantissa = bits&1023u;
    if (exponent == 31)
    {
        throw std::runtime_error("UV diagnostic contains a nonfinite half normal");
    }
    const float value = exponent ? std::ldexp(1.0f+float(mantissa)/1024,int(exponent)-15) :
        std::ldexp(float(mantissa),-24);
    return bits&0x8000u ? -value : value;
}
}

UvSurfacePass::UvSurfacePass(UvSurfaceConfiguration configuration)
    : _configuration(std::move(configuration))
{
    if (_configuration.shaderRoot.empty())
    {
        _configuration.shaderRoot = base::PlatformTools::shaderPath();
    }
}

void UvSurfacePass::initialize(gpu::Device& device, gpu::BackendType backend, uint32_t slots)
{
    if (_device || !device.isValid() || !slots || !_configuration.maxInvocations ||
        _configuration.shaderInterface != "bake.uvSurface.v1")
    {
        throw std::invalid_argument("UV surface needs clean lifecycle, Device/slots and bake.uvSurface.v1");
    }
    const auto caps = device.renderingCapabilities();
    if (caps.maxColorAttachments < 4 ||
        (caps.maxColorAttachmentBytesPerSample && caps.maxColorAttachmentBytesPerSample < 32))
    {
        throw std::runtime_error("UV surface requires four MRTs and 32 attachment bytes per sample");
    }
    for (size_t i = 0; i < Formats.size(); ++i)
    {
        const auto usage = gpu::ImageUsage::Sampled | gpu::ImageUsage::TransferSrc |
            (i < 4 ? gpu::ImageUsage::ColorAttachment : gpu::ImageUsage::Storage);
        if (!device.formatCapabilities(Formats[i]).supports(usage))
        {
            throw std::runtime_error("UV surface native output/coverage format unavailable");
        }
    }
    _device = &device;
    gpu::PipelineLayoutDescription layout;
    layout.resourceBindings.push_back({0,{0,2},gpu::ResourceType::UniformBuffer,gpu::ShaderStage::Vertex});
    layout.resourceBindings.push_back({0,{1,3},gpu::ResourceType::StorageBuffer,gpu::ShaderStage::Vertex});
    _layout = device.createPipelineLayout(layout);
    gpu::ShaderLib library(_configuration.shaderRoot,backend);
    _vertex = library.vertex(_configuration.shaderPrefix+"/surface",&device);
    _fragment = library.fragment(_configuration.shaderPrefix+"/surface",&device);
    gpu::GraphicsPipelineDescription description;
    description.vertexShader = _vertex.get();
    description.fragmentShader = _fragment.get();
    description.layout = _layout.get();
    description.cullMode = gpu::CullMode::None;
    description.depthFormat = gpu::PixelFormat::Undefined;
    description.depthState = gpu::DepthState{false,false,gpu::CompareOperation::Always};
    for (size_t i = 0; i < 4; ++i)
    {
        description.colorAttachments.push_back({Formats[i],{}});
    }
    _pipeline = device.createGraphicsPipeline(description);
    gpu::PipelineLayoutDescription coverage;
    coverage.resourceBindings.push_back({0,{0,0},gpu::ResourceType::SampledImage,gpu::ShaderStage::Compute});
    coverage.resourceBindings.push_back({0,{1,0},gpu::ResourceType::Sampler,gpu::ShaderStage::Compute});
    coverage.resourceBindings.push_back({0,{2,1},gpu::ResourceType::StorageImage,gpu::ShaderStage::Compute});
    _coverageLayout = device.createPipelineLayout(coverage);
    _coverageShader = library.compute(_configuration.shaderPrefix+"/coverage",&device);
    _coveragePipeline = device.createComputePipeline({_coverageShader.get(),_coverageLayout.get()});
    gpu::SamplerDescription sampler;
    sampler.minFilter = sampler.magFilter = gpu::Filter::Nearest;
    sampler.addressModeU = sampler.addressModeV = gpu::AddressMode::ClampToEdge;
    sampler.maxLod = 0;
    _nearest = device.createSampler(sampler);
    _slots.resize(slots);
}

std::shared_ptr<const UvSurfaceGeometry> UvSurfacePass::prepare(std::shared_ptr<draw::Drawable> target)
{
    if (!_pipeline || !target || !target->isLoaded() || !target->engine() || target->engine()->device() != _device ||
        !target->drawMesh() || !target->drawMesh()->vertexBuffer() || target->drawMesh()->vertexBuffer()->device() != _device)
    {
        throw std::invalid_argument("Prepare UV surface from a loaded same-Device draw target");
    }
    _device->waitIdle(); // Explicit rare preparation boundary, never per record.
    auto geometry = std::make_shared<UvSurfaceGeometry>();
    geometry->target = std::move(target);
    geometry->drawMesh = geometry->target->drawMesh();
    const auto& mesh = geometry->drawMesh->meshData();
    const auto limit = _device->resourceArrayCapabilities().maxStorageBufferRange;
    if (!limit || mesh.indices.size() > limit/sizeof(UvSurfaceVertex))
    {
        throw std::length_error("UV packed surface exceeds Device storage-buffer range");
    }
    geometry->packed = buildUvSurfaceMesh(mesh);
    geometry->vertices = _device->createBuffer("bake immutable UV surface vertices");
    geometry->vertices->createStorageBuffer(geometry->packed.vertices);
    return geometry;
}

void UvSurfacePass::beginFrame(uint32_t index, uint64_t serial)
{
    if (!_pipeline || !_coveragePipeline)
    {
        throw std::logic_error("Initialize the UV surface pass before beginning a slot");
    }
    auto& slot = _slots.at(index);
    if (slot.prepared && slot.serial == serial)
    {
        throw std::logic_error("Begin UV surface completed slot once per serial");
    }
    slot.invocations.clear();
    slot.serial = serial;
    slot.prepared = true;
}

UvSurfaceResult UvSurfacePass::record(const UvSurfaceInput& input)
{
    if (!_pipeline || !_coveragePipeline || input.command.hasActiveScope() || !input.geometry ||
        !input.geometry->target || !input.geometry->target->isLoaded() ||
        input.geometry->target->drawMesh() != input.geometry->drawMesh ||
        !input.geometry->vertices || input.geometry->vertices->device() != _device || input.extent.isZero())
    {
        throw std::invalid_argument("UV recording needs current retained geometry, nonzero extent and inactive scope");
    }
    const auto limit = _device->renderingCapabilities().maxImageDimension2D;
    if (input.extent.width > limit || input.extent.height > limit)
    {
        throw std::length_error("UV extent exceeds Device image dimensions");
    }
    auto& slot = _slots.at(input.slot);
    if (!slot.prepared || slot.serial != input.serial || slot.invocations.size() >= _configuration.maxInvocations)
    {
        throw std::logic_error("UV surface slot/serial not prepared or invocation budget exceeded");
    }
    const auto& geometry = *input.geometry;
    if (geometry.packed.ranges.size() != geometry.target->submeshesCount())
    {
        throw std::invalid_argument("UV target submesh topology changed; prepare again");
    }
    // Validate all transforms before recording or allocating output; snapshot once.
    std::vector<SurfaceUniforms> transforms;
    for (uint32_t i = 0; i < geometry.packed.ranges.size(); ++i)
    {
        transforms.push_back(transform(input.nodeWorld*geometry.target->submeshTransform(i)));
        const auto range = geometry.packed.ranges[i];
        for (uint32_t j = 0; j < range.indexCount; ++j)
        {
            const auto& vertex = geometry.packed.vertices.at(size_t(range.firstIndex)+j);
            const auto position = transforms.back().model*vertex.position;
            const auto normal = glm::mat3(transforms.back().normal)*glm::vec3(vertex.normal);
            const float length = glm::length(normal);
            for (uint32_t axis = 0; axis < 3; ++axis)
            {
                if (!std::isfinite(position[axis]) || !std::isfinite(normal[axis]) || !std::isfinite(length) || length <= 1e-12f)
                {
                    throw std::invalid_argument("UV transformed position/normal cannot be represented");
                }
            }
        }
    }
    // Retain before recording so partial recording errors cannot free referenced owners.
    slot.invocations.emplace_back();
    auto& invocation = slot.invocations.back();
    auto& result = invocation.result;
    result.geometry = input.geometry;
    result.sourceGeneration = input.sourceGeneration;
    result.targetGeneration = input.targetGeneration;
    for (size_t i = 0; i < Formats.size(); ++i)
    {
        gpu::ImageDescription description;
        description.size = input.extent;
        description.format = Formats[i];
        description.usage = gpu::ImageUsage::Sampled | gpu::ImageUsage::TransferSrc |
            (i < 4 ? gpu::ImageUsage::ColorAttachment : gpu::ImageUsage::Storage);
        result.images[i] = _device->createImage(description);
    }
    gpu::RenderingDescription rendering;
    for (size_t i = 0; i < 4; ++i)
    {
        input.command.transition(result.images[i].get(),gpu::ImageLayout::ColorAttachment);
        gpu::ColorAttachmentDescription color;
        color.image = result.images[i].get();
        color.load = gpu::AttachmentLoadOperation::Clear;
        color.clear.floatingPoint = {0,0,0,0};
        if (i == 2)
        {
            color.clear.unsignedInteger = {std::numeric_limits<uint32_t>::max(),0,0,0};
        }
        rendering.colors.push_back(color);
    }
    // Capture immutable per-draw descriptors before opening native rendering.
    for (const auto& data : transforms)
    {
        auto uniform = _device->createBuffer("bake UV transform");
        uniform->createUniformBuffer(data);
        invocation.uniforms.push_back(uniform);
        auto set = _device->createResourceSet(_layout.get(),0);
        set->setUniformBuffer({0,2},uniform.get());
        set->setStorageBuffer({1,3},geometry.vertices.get());
        set->update();
        invocation.sets.push_back(set);
    }
    input.command.beginRendering(rendering);
    input.command.bindPipeline(_pipeline.get());
    for (size_t i = 0; i < geometry.packed.ranges.size(); ++i)
    {
        const auto range = geometry.packed.ranges[i];
        if (range.indexCount)
        {
            input.command.bindResourceSet(_pipeline.get(),0,invocation.sets[i].get());
            input.command.draw(range.indexCount,1,range.firstIndex,0);
        }
    }
    input.command.endRendering();
    for (size_t i = 0; i < 4; ++i)
    {
        input.command.transition(result.images[i].get(),gpu::ImageLayout::ShaderReadOnly);
    }
    auto coverage = _device->createResourceSet(_coverageLayout.get(),0);
    invocation.sets.push_back(coverage);
    coverage->setSampledImage({0,0},result.images[3].get());
    coverage->setSampler({1,0},_nearest.get());
    coverage->setStorageImage({2,1},result.images[4].get());
    coverage->update();
    input.command.transition(result.images[4].get(),gpu::ImageLayout::General);
    input.command.beginCompute();
    input.command.bindPipeline(_coveragePipeline.get());
    input.command.bindResourceSet(_coveragePipeline.get(),0,coverage.get());
    input.command.dispatch((input.extent.width+15)/16,(input.extent.height+15)/16,1);
    input.command.endCompute();
    input.command.transition(result.images[4].get(),gpu::ImageLayout::ShaderReadOnly);
    return result;
}

void UvSurfacePass::cleanup()
{
    if (_device)
    {
        _device->waitIdle();
    }
    _slots.clear();
    _pipeline.reset();
    _coveragePipeline.reset();
    _layout.reset();
    _coverageLayout.reset();
    _vertex.reset();
    _fragment.reset();
    _coverageShader.reset();
    _nearest.reset();
    _device = nullptr;
}

// No automatic GPU completion can be inferred from CPU result ownership.
UvSurfaceDiagnostic UvSurfacePass::readDiagnostic(const UvSurfaceResult& result)
{
    const auto surface = result.surfaceInputs();
    UvSurfaceDiagnostic output;
    output.extent = surface.extent();
    const size_t pixels = size_t(output.extent.width)*output.extent.height;
    output.positions.resize(pixels);
    output.normals.resize(pixels);
    output.submeshes.resize(pixels);
    output.charts.resize(pixels);
    output.coverage.resize(pixels);
    for (size_t i = 0; i < Formats.size(); ++i)
    {
        const auto& image = result.images[i];
        if (!image || image->size() != output.extent || image->pixelFormat() != Formats[i] ||
            image->device() != surface.normal->device())
        {
            throw std::invalid_argument("UV diagnostic image profile mismatch");
        }
        std::vector<uint8_t> bytes;
        image->readPixels(bytes,{},gpu::ImageLayout::ShaderReadOnly);
        if (bytes.size() != pixels*gpu::pixelFormatBytes(Formats[i]))
        {
            throw std::runtime_error("UV diagnostic raw byte count mismatch");
        }
        if (i == 0)
        {
            static_assert(sizeof(glm::vec4) == 16 && sizeof(float) == 4 && std::numeric_limits<float>::is_iec559);
            std::memcpy(output.positions.data(),bytes.data(),bytes.size());
        }
        else if (i == 1)
        {
            for (size_t pixel = 0; pixel < pixels; ++pixel)
            {
                for (size_t channel = 0; channel < 4; ++channel)
                {
                    uint16_t bits;
                    std::memcpy(&bits,bytes.data()+(pixel*4+channel)*2,2);
                    output.normals[pixel][channel] = halfFloat(bits);
                }
            }
        }
        else if (i == 2 || i == 3)
        {
            auto& values = i == 2 ? output.submeshes : output.charts;
            std::memcpy(values.data(),bytes.data(),bytes.size());
        }
        else
        {
            output.coverage = std::move(bytes);
        }
    }
    return output;
}

draw::pipeline::SurfaceInputs UvSurfaceResult::surfaceInputs() const
{
    if (!images[0] || !images[1] || !images[4])
    {
        throw std::invalid_argument("UV result has no explicit surface owners");
    }
    draw::pipeline::SurfaceInputs result;
    result.profile = draw::pipeline::SurfaceProfile::WorldPositionCoverage;
    result.normal = images[1].get();
    result.depthOrPosition = images[0].get();
    result.coverage = images[4].get();
    result.validate(*images[0]->device());
    return result;
}
}
