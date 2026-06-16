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

#include <bg2e/common.hpp>
#include <bg2e/gpu/Common.hpp>
#include <bg2e/gpu/DeviceResource.hpp>

#include <cstdint>
#include <memory>

namespace bg2e {
namespace gpu {

class Image;
class CubeMap;
class Sampler;
class Buffer;

/**
 * @brief ResourceSet binds shader resources (textures, samplers, uniform/storage
 *        buffers) to a pipeline layout.
 *
 * ## ShaderBinding: Vulkan vs Metal indices
 *
 * Every resource binding is described by a ShaderBinding with two indices:
 *
 *   ShaderBinding {
 *       uint32_t vulkan;   // Vulkan descriptor binding index (set + binding)
 *       uint32_t metal;    // Metal [[buffer(N)]], [[texture(N)]], [[sampler(N)]]
 *   };
 *
 * Vulkan descriptor set/binding indices and Metal resource indices are different
 * concepts. `BackendBinding.vulkan` maps to Vulkan descriptor binding.
 * `BackendBinding.metal` maps to Metal buffer(n), texture(n) or sampler(n)
 * depending on the resource type.
 *
 * ## Vulkan binding model
 *
 * In Vulkan, resources are organised by *descriptor set* and *binding* index
 * within each set. The pipeline layout declares a set of (set, binding) pairs
 * and the VkDescriptorSetLayout maps each binding to a descriptor type.
 *
 * Push constants do NOT use ShaderBinding. They are passed via vkCmdPushConstants
 * and do not occupy a binding index. The offset field in PushConstantRange is the
 * Vulkan push constant offset.
 *
 *   // GLSL — set = 0, binding = 0 → uniform buffer
 *   layout(set = 0, binding = 0) uniform CameraUBO {
 *       mat4 projection;
 *       mat4 view;
 *   } camera;
 *
 *   // C++ layout declaration
 *   layoutDesc.resourceBindings.push_back(
 *       { 0, {.vulkan = 0, .metal = 2}, ResourceType::UniformBuffer,
 *         ShaderStage::Vertex, 1 });
 *
 *   // C++ resource set — binding.vulkan = 0 matches GLSL binding = 0
 *   cameraSet->setUniformBuffer({.vulkan = 0, .metal = 2}, cameraUbo);
 *
 * ## Metal binding model
 *
 * Metal argument buffers use three *independent* index namespaces:
 *   - [[buffer(N)]]   — uniform/storage buffers + push constants
 *   - [[texture(N)]]  — sampled/storage images
 *   - [[sampler(N)]]  — samplers
 *
 * A [[texture(0)]], [[sampler(0)]] and [[buffer(0)]] can coexist without
 * collision because they belong to different namespaces.
 *
 * ### Metal push constant buffer indices
 *
 * Push constants do NOT use ShaderBinding. Metal push constant buffer indices
 * are fixed by bg2e::gpu convention:
 *
 *   Vertex   push constants -> [[buffer(1)]]
 *   Fragment push constants -> [[buffer(0)]]
 *   Compute  push constants -> [[buffer(0)]]
 *
 * In Metal vertex shaders, [[buffer(0)]] is reserved for the geometric vertex
 * buffer because bg2e::gpu always binds vertex data as a single vertex buffer.
 *
 * ### Metal resource buffer binding rules
 *
 * For UniformBuffer and StorageBuffer bindings declared through resourceBindings,
 * the Metal buffer index must respect reserved slots:
 *
 *   Vertex stage:   metal index >= 2  (buffer(0)=geometry, buffer(1)=push constants)
 *   Fragment stage: metal index >= 1  (buffer(0)=push constants)
 *   Compute stage:  metal index >= 1  (buffer(0)=push constants)
 *
 * These restrictions do NOT apply to SampledImage, StorageImage, or Sampler
 * bindings because Metal buffer, texture and sampler indices are separate
 * namespaces. Texture and sampler indices can start at 0.
 *
 * ### Metal example — vertex push constants + vertex UBO + fragment texture
 *
 *   // Metal vertex shader
 *   constant CameraPushConstants& cam  [[buffer(1)]];
 *   constant ModelUBO&           model [[buffer(2)]];
 *
 *   // Metal fragment shader
 *   constant FragmentPushConstants& push [[buffer(0)]];
 *   constant FragmentSettings& settings [[buffer(1)]];
 *   texturecube<float> cubeMap  [[texture(0)]];
 *   sampler           cubeSampler [[sampler(0)]];
 *
 *   // C++ layout declaration
 *   layoutDesc.pushConstants.push_back(
 *       {0, sizeof(CameraPushConstants), gpu::ShaderStage::Vertex});
 *   layoutDesc.pushConstants.push_back(
 *       {0, sizeof(FragmentPushConstants), gpu::ShaderStage::Fragment});
 *   layoutDesc.resourceBindings.push_back(
 *       { 0, {.vulkan = 0, .metal = 2}, ResourceType::UniformBuffer,
 *         ShaderStage::Vertex, 1 });    // camera → buffer(2)
 *   layoutDesc.resourceBindings.push_back(
 *       { 0, {.vulkan = 0, .metal = 1}, ResourceType::UniformBuffer,
 *         ShaderStage::Fragment, 1 });  // settings → buffer(1)
 *   layoutDesc.resourceBindings.push_back(
 *       { 1, {.vulkan = 0, .metal = 0}, ResourceType::SampledImage,
 *         ShaderStage::Fragment, 1 });  // cubeMap → texture(0)
 *   layoutDesc.resourceBindings.push_back(
 *       { 1, {.vulkan = 1, .metal = 0}, ResourceType::Sampler,
 *         ShaderStage::Fragment, 1 });  // cubeSampler → sampler(0)
 *
 * ## Summary table
 *
 *   Resource type         Metal namespace   Vertex idx   Fragment idx   Compute idx
 *   ───────────────────── ───────────────── ──────────── ────────────── ───────────
 *   Push constants        [[buffer]]        1            0              0
 *   Geometric vertex buf  [[buffer]]        0            —              —
 *   Uniform/Storage buf   [[buffer]]        2, 3, ...    1, 2, ...      1, 2, ...
 *   Sampled/Storage img   [[texture]]       0, 1, ...    0, 1, ...      0, 1, ...
 *   Sampler               [[sampler]]       0, 1, ...    0, 1, ...      0, 1, ...
 *
 *   Vulkan has no concept of push constants at the binding level; they are
 *   passed separately via vkCmdPushConstants and do not occupy a binding index.
 */
class BG2E_API ResourceSet : public DeviceResource {
public:
    explicit ResourceSet(Device* device) : DeviceResource(device) {}
    virtual ~ResourceSet() = default;

    virtual void setStorageImage(ShaderBinding binding, gpu::Image* image) = 0;
    virtual void setSampledImage(ShaderBinding binding, gpu::Image* image) = 0;
    virtual void setSampler(ShaderBinding binding, gpu::Sampler* sampler)  = 0;
    virtual void setUniformBuffer(ShaderBinding binding, gpu::Buffer* buffer) = 0;
    virtual void setStorageBuffer(ShaderBinding binding, gpu::Buffer* buffer) = 0;

    void setSampledCubeMap(ShaderBinding binding, gpu::CubeMap* cubeMap);

    // Convenience overloads for shared_ptr-owned buffers (e.g. FrameResourceRing).
    void setUniformBuffer(ShaderBinding binding, const std::shared_ptr<gpu::Buffer>& buffer)
    {
        setUniformBuffer(binding, buffer.get());
    }
    void setStorageBuffer(ShaderBinding binding, const std::shared_ptr<gpu::Buffer>& buffer)
    {
        setStorageBuffer(binding, buffer.get());
    }

    virtual void update() = 0;   // flush assignments to the backend

    virtual uint32_t setIndex() const = 0;
    virtual bool isValid() const = 0;
    virtual void cleanup() = 0;
};

}
}
