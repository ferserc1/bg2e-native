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

#include <bg2e/gpu/ComputePipeline.hpp>
#include <bg2e/gpu/vk/common.hpp>

namespace bg2e {
namespace gpu {
namespace vk {

class BG2E_API ComputePipeline : public gpu::ComputePipeline {
public:
    ComputePipeline(VkDevice device, const gpu::ComputePipelineDescription& description);
    ~ComputePipeline() override;

    bool isValid() const override;
    void cleanup() override;

    VkPipeline handle() const { return _pipeline; }
    VkPipelineBindPoint bindPoint() const { return _bindPoint; }
    VkPipelineLayout layoutHandle() const { return _layoutHandle; }

private:
    VkDevice _device{VK_NULL_HANDLE};
    VkPipeline _pipeline{VK_NULL_HANDLE};
    VkPipelineBindPoint _bindPoint{VK_PIPELINE_BIND_POINT_COMPUTE};
    VkPipelineLayout _layoutHandle{VK_NULL_HANDLE};
};

}
}
}
