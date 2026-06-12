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

#include <bg2e/gpu/Device.hpp>
#include <bg2e/gpu/metal/common.hpp>
#include <bg2e/gpu/metal/Queue.hpp>

#include <functional>

namespace bg2e {
namespace gpu {
namespace metal {

class Device : public gpu::Device {
public:
    void create(gpu::Instance* instance, gpu::PhysicalDevice* physicalDevice, gpu::Surface* surface) override;
    void cleanup() override;
    void waitIdle() override;

    bool isValid() const override;

    const gpu::Queue& graphicsQueue() const override;
    const gpu::Queue& presentQueue() const override;
    const gpu::Queue& transferQueue() const override;

    std::unique_ptr<gpu::ShaderModule> createShaderModule(const gpu::ShaderModuleDescription& description) override;
    std::unique_ptr<gpu::PipelineLayout> createPipelineLayout(const gpu::PipelineLayoutDescription& description) override;
    std::unique_ptr<gpu::GraphicsPipeline> createGraphicsPipeline(const gpu::GraphicsPipelineDescription& description) override;
    std::unique_ptr<gpu::ComputePipeline> createComputePipeline(const gpu::ComputePipelineDescription& description) override;
    std::shared_ptr<gpu::Sampler> createSampler(const gpu::SamplerDescription& description) override;
    std::unique_ptr<gpu::ResourceSet> createResourceSet(gpu::PipelineLayout* layout, uint32_t setIndex) override;
    std::shared_ptr<gpu::Image> createImage(const ImageDescription& description) override;

    void immediateSubmit(std::function<void(gpu::CommandBuffer* cmd)>&& function) override;

    DeviceHandle handle() const { return _device; }

private:
    DeviceHandle _device = nullptr;
    metal::Queue _graphicsQueue;
    metal::Queue _presentQueue;
    metal::Queue _transferQueue;
};

}
}
}
