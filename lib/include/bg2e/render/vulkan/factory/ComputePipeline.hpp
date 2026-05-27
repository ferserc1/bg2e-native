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
#include <bg2e/render/vulkan/common.hpp>
#include <bg2e/render/Engine.hpp>

namespace bg2e {
namespace render {
namespace vulkan {
namespace factory {

class BG2E_API ComputePipeline {
public:
    ComputePipeline(Engine * engine);
    ~ComputePipeline();

    void setShader(const std::string& fileName, const std::string& entryPoint = "main", const std::string& basePath = "");
    void setShader(VkShaderModule shaderModule, const std::string& entryPoint = "main");

    VkPipeline build(VkPipelineLayout layout, const std::string& name);

protected:
    Engine * _engine;

    VkShaderModule _shaderModule;
    std::string _shaderEntryPoint = "main";
    VkPipelineShaderStageCreateInfo _shaderStageInfo = {};
};

}
}
}
}
