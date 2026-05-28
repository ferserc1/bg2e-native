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

#include <bg2e/common.hpp>
#include <bg2e/render/Engine.hpp>
#include <bg2e/render/vulkan/common.hpp>
#include <bg2e/render/vulkan/Buffer.hpp>

#include <memory>
#include <string>
#include <vector>

namespace bg2e {
namespace render {
namespace vulkan {
namespace factory {

class BG2E_API RayTracingPipeline {
public:
    explicit RayTracingPipeline(Engine* engine);
    ~RayTracingPipeline();

    void setRayGenShader(const std::string& fileName, const std::string& entryPoint = "main");
    void setMissShader(const std::string& fileName, const std::string& entryPoint = "main");
    void setClosestHitShader(const std::string& fileName, const std::string& entryPoint = "main");

    VkPipeline build(
        VkPipelineLayout layout,
        uint32_t maxRecursionDepth = 1,
        const std::string& name = ""
    );

    struct SBTData {
        std::unique_ptr<vulkan::Buffer> buffer;
        VkStridedDeviceAddressRegionKHR raygenRegion = {};
        VkStridedDeviceAddressRegionKHR missRegion = {};
        VkStridedDeviceAddressRegionKHR hitRegion = {};
        VkStridedDeviceAddressRegionKHR callableRegion = {};
    };

    SBTData createSBT(const std::string& name = "");

    VkPipeline pipeline() const { return _pipeline; }
    uint32_t groupCount() const { return static_cast<uint32_t>(_groups.size()); }

    // Reset the factory to use it again to create another pipeline
    void reset();

private:
    Engine* _engine = nullptr;
    VkPipeline _pipeline = VK_NULL_HANDLE;

    std::vector<VkPipelineShaderStageCreateInfo> _stages;
    std::vector<VkRayTracingShaderGroupCreateInfoKHR> _groups;
    std::vector<VkShaderModule> _shaderModules;

    uint32_t _raygenGroupIndex = 0;
    uint32_t _missGroupIndex = 0;
    uint32_t _hitGroupIndex = 0;

    bool _hasRaygenShader = false;
    bool _hasMissShader = false;
    bool _hasClosestHitShader = false;

    uint32_t addShaderStage(
        VkShaderStageFlagBits stage,
        const std::string& fileName,
        const std::string& entryPoint
    );

    static VkDeviceSize alignUp(VkDeviceSize value, VkDeviceSize alignment);
};

}
}
}
}