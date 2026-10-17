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

#pragma once

#include <bg2e/gpu/CommandBuffer.hpp>
#include <bg2e/gpu/Device.hpp>
#include <bg2e/gpu/Image.hpp>
#include <bg2e/gpu/ResourceSet.hpp>
#include <bg2e/gpu/RayTracingScene.hpp>
#include <bg2e/gpu/ShaderLib.hpp>
#include <glm/glm.hpp>
#include <array>
#include <cmath>
#include <filesystem>
#include <stdexcept>

namespace bg2e::gpu::tests {
// Run with a stopped initialized Device and the deployed shaders root. false
// means unsupported, never a successful dispatch. No window/scene/draw API.
inline bool runAdvancedTracingProbe(Device& device, BackendType backend, const std::filesystem::path& root)
{
    const auto caps = device.resourceArrayCapabilities();
    const auto count = caps.profileCapacity(0, 0, 6);
    const auto rt = device.rayTracingCapabilities();
    if (!count || (!rt.querySupported() && (backend == BackendType::Metal || !rt.pipelineSupported())))
    {
        return false;
    }
    const auto outputCaps = device.formatCapabilities(PixelFormat::R8G8B8A8_UNORM);
    if (!outputCaps.supports(ImageUsage::Storage | ImageUsage::TransferSrc | ImageUsage::Sampled | ImageUsage::TransferDst))
    {
        return false;
    }
    struct alignas(16) Vertex {
        glm::vec4 position, normal, uv, tangent;
    };
    struct alignas(16) Instance {
        glm::mat4 model{1}, normal{1};
        glm::uvec4 ranges{0}, identity{0};
    };
    struct alignas(16) Material {
        glm::uvec4 images{0};
        glm::vec4 parameters{0};
    };
    static_assert(sizeof(Vertex) == 64 && sizeof(Instance) == 160 && sizeof(Material) == 32);
    std::vector<Vertex> vertices;
    for (float z : {1.0f, 2.0f})
    {
        for (auto xy : {glm::vec2(-1,-1), glm::vec2(1,-1), glm::vec2(0,1)})
        {
            vertices.push_back({glm::vec4(xy,z,1), {0,0,-1,0}, {0.25f,0.5f,z == 1.0f ? 1.25f : 0.25f,0.5f}, {1,0,0,1}});
        }
    }
    // Nonzero firstIndex, local indices and a separate vertex base in packed data.
    const std::vector<uint32_t> indices = {3,4,5, 0,1,2, 3,4,5};
    auto vertexBuffer = device.createBuffer("probe BLAS vertices");
    auto indexBuffer = device.createBuffer("probe BLAS indices");
    vertexBuffer->createVertexBuffer(vertices);
    indexBuffer->createIndexBuffer(indices);
    std::array<std::shared_ptr<RayTracingMesh>,2> meshes;
    for (uint32_t i = 0; i < 2; ++i)
    {
        RayTracingMeshDescription description;
        description.vertexBuffer = vertexBuffer.get();
        description.indexBuffer = indexBuffer.get();
        description.vertexCount = 6;
        description.vertexStride = sizeof(Vertex);
        description.firstIndex = 3 + i * 3;
        description.indexCount = 3;
        meshes[i] = device.createRayTracingMesh(description);
    }
    auto scene = device.createRayTracingScene("primitive cutout probe");
    scene->addInstance(meshes[0].get(), glm::mat4(1), 0, 255, RayTracingOpacity::ForceNonOpaque);
    scene->addInstance(meshes[1].get(), glm::mat4(1), 1, 255, RayTracingOpacity::ForceOpaque);
    device.immediateSubmit([&](CommandBuffer* command)
    {
        for (const auto& mesh : meshes)
        {
            command->buildRayTracingMesh(mesh.get());
        }
        scene->buildOrUpdate(command);
    });
    std::vector<Instance> instances(2);
    instances[0].ranges = {3,3,1,0};
    instances[1].ranges = {6,3,1,1};
    instances[0].identity = {41,0,0,0};
    instances[1].identity = {93,1,0,0};
    std::vector<Material> materials(2);
    materials[0].images = {0,30,0,1}; // UV1 repeats/clamps differently from UV0.
    materials[0].parameters.x = 0.5f;
    materials[1].images = {30,1,0,1};
    std::array<std::shared_ptr<Buffer>,4> tables;
    for (auto& table : tables)
    {
        table = device.createBuffer("packed primitive tracing data");
    }
    std::vector<Vertex> packedVertices(3);
    packedVertices.insert(packedVertices.end(),vertices.begin(),vertices.end());
    tables[0]->createStorageBuffer(packedVertices);
    tables[1]->createStorageBuffer(indices);
    tables[2]->createStorageBuffer(instances);
    tables[3]->createStorageBuffer(materials);
    std::vector<std::shared_ptr<Image>> owners;
    std::vector<ImageView> views;
    for (uint32_t i = 0; i < count; ++i)
    {
        ImageDescription description;
        description.size = {2,1};
        description.format = PixelFormat::R8G8B8A8_UNORM;
        description.usage = ImageUsage::Sampled | ImageUsage::TransferDst;
        auto image = device.createImage(description);
        const auto intensity = static_cast<uint8_t>(8 + (i % 32) * 7);
        const auto alpha = static_cast<uint8_t>(i % 2 ? 255 : 0);
        const std::array<uint8_t,8> pixels = {intensity,0,0,alpha, 0,intensity,0,alpha};
        image->upload({pixels.data(), pixels.size(), 8,0,0});
        views.push_back(image->fullView());
        owners.push_back(image);
    }
    SamplerDescription samplerDescription;
    samplerDescription.minFilter = Filter::Nearest;
    samplerDescription.magFilter = Filter::Nearest;
    auto repeat = device.createSampler(samplerDescription);
    samplerDescription.addressModeU = AddressMode::ClampToEdge;
    auto clamp = device.createSampler(samplerDescription);
    std::vector<Sampler*> samplers(count, repeat.get());
    samplers[1] = clamp.get();
    ImageDescription outputDescription;
    outputDescription.size = {32,1};
    outputDescription.format = PixelFormat::R8G8B8A8_UNORM;
    outputDescription.usage = ImageUsage::Storage | ImageUsage::TransferSrc;
    auto output = device.createImage(outputDescription);
    ShaderLib library(root / "draw/rt_probe", backend);
    const auto name = "probe" + std::to_string(count);
    auto checkPixels = [&]
    {
        std::vector<uint8_t> pixels;
        output->readPixelsRGBA8(pixels, ImageLayout::General);
        if (pixels.size() != 32 * 4)
        {
            throw std::runtime_error("Tracing probe output extent mismatch");
        }
        for (uint32_t x = 0; x < 32; ++x)
        {
            const auto front = x % 30;
            const auto image = front % 2 ? front : 30u;
            const int intensity = 8 + image * 7;
            const bool green = front % 2 && x % 3 == 0;
            const int expected[] = {green ? 0 : intensity, green ? intensity : 0, 0, 255};
            for (uint32_t channel = 0; channel < 4; ++channel)
            {
                if (std::abs(int(pixels[x*4+channel]) - expected[channel]) > 1)
                {
                    throw std::runtime_error("Tracing probe cutout/index/UV/nonuniform sampler pixel mismatch");
                }
            }
        }
    };
    auto run = [&](bool pipelinePath)
    {
        const auto stage = pipelinePath ? ShaderStage::RayGeneration : ShaderStage::Compute;
        PipelineLayoutDescription layoutDescription;
        layoutDescription.resourceBindings = {
            {0,{0,0},ResourceType::StorageImage,stage},
            {0,{1,0},ResourceType::SampledImage,stage,count,{},6},
            {0,{2,count},ResourceType::Sampler,stage,count,{},6},
            {0,{3,1},ResourceType::StorageBuffer,stage},
            {0,{4,2},ResourceType::StorageBuffer,stage},
            {0,{5,3},ResourceType::StorageBuffer,stage},
            {0,{6,4},ResourceType::StorageBuffer,stage},
            {0,{7,5},ResourceType::AccelerationStructure,stage}};
        if (pipelinePath)
        {
            for (auto& binding : layoutDescription.resourceBindings)
            {
                binding.additionalStages = {ShaderStage::ClosestHit,ShaderStage::AnyHit};
            }
        }
        auto tooLarge = layoutDescription;
        tooLarge.resourceBindings[1].count = caps.maxSampledImages + 1;
        bool capacityRejected = false;
        try
        {
            device.createPipelineLayout(tooLarge);
        }
        catch (const std::exception&)
        {
            capacityRejected = true;
        }
        if (!capacityRejected)
        {
            throw std::runtime_error("Oversized tracing table layout was accepted");
        }
        auto layout = device.createPipelineLayout(layoutDescription);
        auto resources = device.createResourceSet(layout.get(),0);
        auto mustReject = [](auto operation)
        {
            bool rejected = false;
            try
            {
                operation();
            }
            catch (const std::exception&)
            {
                rejected = true;
            }
            if (!rejected)
            {
                throw std::runtime_error("Tracing resource validation failed to reject invalid input");
            }
        };
        mustReject([&]
        {
            resources->setSampledImages({1,0}, std::span<const ImageView>(views.data(),count-1));
        });
        mustReject([&]
        {
            resources->setSamplers({2,count}, std::span<Sampler* const>(samplers.data(),count-1));
        });
        auto invalidViews = views;
        invalidViews[0].image = nullptr;
        mustReject([&]
        {
            resources->setSampledImages({1,0}, invalidViews);
        });
        auto invalidSamplers = samplers;
        invalidSamplers[0] = nullptr;
        mustReject([&]
        {
            resources->setSamplers({2,count},invalidSamplers);
        });
        resources->setStorageImage({0,0},output.get());
        for (uint32_t i = 0; i < tables.size(); ++i)
        {
            resources->setStorageBuffer({3+i,1+i},tables[i].get());
        }
        resources->setRayTracingScene({7,5},scene.get());
        mustReject([&]
        {
            resources->update(); // No array may be left uninitialized.
        });
        resources->setSampledImages({1,0},views);
        resources->setSamplers({2,count},samplers);
        resources->update();
        if (pipelinePath)
        {
            auto raygen = library.rayGeneration(name,&device);
            auto miss = library.miss(name,&device);
            auto hit = library.closestHit(name,&device);
            auto any = library.anyHit(name,&device);
            RayTracingPipelineDescription description;
            description.raygenShader = raygen.get();
            description.missShader = miss.get();
            description.closestHitShader = hit.get();
            description.anyHitShader = any.get();
            description.layout = layout.get();
            // Missing miss group must not shift the hit SBT outside its allocation.
            auto noMiss = description;
            noMiss.missShader = nullptr;
            auto noMissPipeline = device.createRayTracingPipeline(noMiss);
            auto wrongStage = description;
            wrongStage.anyHitShader = hit.get();
            mustReject([&]
            {
                device.createRayTracingPipeline(wrongStage);
            });
            auto pipeline = device.createRayTracingPipeline(description);
            device.immediateSubmit([&](CommandBuffer* command)
            {
                command->transition(output.get(),ImageLayout::General);
                command->bindPipeline(pipeline.get());
                command->bindResourceSet(pipeline.get(),0,resources.get());
                command->traceRays(32,1);
            });
        }
        else
        {
            auto shader = library.compute(name,&device);
            auto pipeline = device.createComputePipeline({shader.get(),layout.get()});
            device.immediateSubmit([&](CommandBuffer* command)
            {
                command->transition(output.get(),ImageLayout::General);
                command->beginCompute();
                command->bindPipeline(pipeline.get());
                command->bindResourceSet(pipeline.get(),0,resources.get());
                command->dispatch(2,1,1);
                command->endCompute();
            });
        }
        checkPixels();
        // Valid updates at this completed-submission boundary must remain usable.
        resources->setSamplers({2,count},samplers);
        resources->update();
    };
    if (rt.querySupported())
    {
        run(false);
    }
    if (backend == BackendType::Vulkan && rt.pipelineSupported())
    {
        run(true);
    }
    return true;
}
}
