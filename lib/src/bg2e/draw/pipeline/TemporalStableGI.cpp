/*
 *    business grade graphic engine (bg2e engine)
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

#include <bg2e/draw/pipeline/TemporalAccumulator.hpp>
#include <bg2e/gpu/ShaderLib.hpp>
#include <array>

namespace bg2e::draw::pipeline {
struct TemporalAccumulator::StableGIState {
    std::array<std::shared_ptr<gpu::ShaderModule>,15> shaders;
    std::array<std::shared_ptr<gpu::ComputePipeline>,15> pipelines;
    std::shared_ptr<gpu::ShaderModule> presentShader;
    std::shared_ptr<gpu::PipelineLayout> presentLayout;
    std::shared_ptr<gpu::ComputePipeline> presentPipeline;
};

namespace {
struct alignas(16) PresentUniforms {
    glm::mat4 inverseViewProjection{1}, stableViewProjection{1};
    glm::uvec4 extent{0};
    glm::vec4 rejection{0}, options{0};
};
static_assert(sizeof(PresentUniforms) == 176);
}

gpu::ComputePipeline* TemporalAccumulator::stableGIPipeline(uint32_t mode, bool diagnose, bool metrics,
    const std::shared_ptr<gpu::PipelineLayout>& layout)
{
    if (!_stableGI)
    {
        _stableGI = std::make_shared<StableGIState>();
    }
    const auto index = mode*3u+(metrics ? 2u : diagnose ? 1u : 0u);
    auto& pipeline = _stableGI->pipelines.at(index);
    if (!pipeline)
    {
        gpu::ShaderLib library(_configuration.shaderRoot,_backend);
        const auto name = "draw/temporal_stable_gi/v"+std::to_string(mode+1)+
            (metrics ? "_metrics" : diagnose ? "_diagnostic" : "");
        auto shader = library.compute(name,_device);
        pipeline = _device->createComputePipeline({shader.get(),layout.get(),name+"-pipeline"});
        _stableGI->shaders.at(index) = std::move(shader);
    }
    return pipeline.get();
}

std::shared_ptr<gpu::Image> TemporalAccumulator::presentStableGI(const TemporalInput& input,
    const std::shared_ptr<History>& history, const std::shared_ptr<gpu::Image>& signal,
    bool diagnostic, bool nearest)
{
    if (!_stableGI->presentPipeline)
    {
        gpu::PipelineLayoutDescription layout;
        layout.debugName = "temporal-stable-gi-presentation-layout";
        layout.resourceBindings.push_back({0,{0,1},gpu::ResourceType::UniformBuffer,gpu::ShaderStage::Compute});
        for (uint32_t i = 1; i <= 6; ++i)
        {
            layout.resourceBindings.push_back({0,{i,i-1},gpu::ResourceType::SampledImage,gpu::ShaderStage::Compute});
        }
        layout.resourceBindings.push_back({0,{7,0},gpu::ResourceType::Sampler,gpu::ShaderStage::Compute});
        layout.resourceBindings.push_back({0,{8,6},gpu::ResourceType::StorageImage,gpu::ShaderStage::Compute});
        _stableGI->presentLayout = _device->createPipelineLayout(layout);
        gpu::ShaderLib library(_configuration.shaderRoot,_backend);
        _stableGI->presentShader = library.compute("draw/temporal_stable_gi/present",_device);
        _stableGI->presentPipeline = _device->createComputePipeline({_stableGI->presentShader.get(),
            _stableGI->presentLayout.get(),"temporal-stable-gi-presentation-pipeline"});
    }
    gpu::ImageDescription description;
    description.debugName = diagnostic ? "temporal-stable-gi-diagnostic-presentation" : "temporal-stable-gi-render-presentation";
    description.size = input.signal.size();
    description.format = gpu::PixelFormat::R16G16B16A16_SFLOAT;
    description.usage = gpu::ImageUsage::Sampled | gpu::ImageUsage::Storage | gpu::ImageUsage::TransferSrc;
    auto output = _pool->image(input.command,description);
    PresentUniforms uniforms;
    uniforms.inverseViewProjection = input.surface.inverseViewProjection;
    uniforms.stableViewProjection = input.view.projection*input.view.view;
    uniforms.extent = {input.surface.extent().width,input.surface.extent().height,input.signal.width(),input.signal.height()};
    uniforms.rejection = {_settings.absolutePositionThreshold,_settings.relativePositionThreshold,_settings.normalThreshold,nearest ? 1.0f : 0.0f};
    uniforms.options = glm::vec4(glm::vec3(glm::inverse(input.view.view)[3]),diagnostic ? 1.0f : 0.0f);
    auto pooled = _pool->bindings(input.command,_stableGI->presentLayout,0,sizeof(uniforms),
        {input.slot,0x5354474900000000ull | uint64_t(input.key.signal),input.key.view,input.key.layer});
    pooled->writeUniforms(&uniforms,sizeof(uniforms));
    pooled->setUniformBuffer({0,1},pooled->uniforms().get());
    const std::array<gpu::Image*,6> images{signal.get(),history->position.get(),history->normal.get(),
        input.surface.normal,input.surface.depthOrPosition,&input.signal};
    for (uint32_t i = 0; i < images.size(); ++i)
    {
        input.command.transition(images[i],gpu::ImageLayout::ShaderReadOnly);
        pooled->setSampledImage({i+1,i},images[i]);
    }
    pooled->setSampler({7,0},_nearest.get());
    input.command.transition(output.get(),gpu::ImageLayout::General);
    pooled->setStorageImage({8,6},output.get());
    pooled->update();
    auto& invocation = _slots.at(input.slot).invocations.back();
    invocation.stableOwners.push_back(signal);
    invocation.stableOwners.push_back(output);
    invocation.stableBindings.push_back(pooled);
    input.command.beginCompute();
    input.command.bindPipeline(_stableGI->presentPipeline.get());
    input.command.bindResourceSet(_stableGI->presentPipeline.get(),0,pooled->resources().get());
    input.command.dispatch((output->width()+15)/16,(output->height()+15)/16,1);
    input.command.endCompute();
    input.command.transition(output.get(),gpu::ImageLayout::ShaderReadOnly);
    return output;
}
}
