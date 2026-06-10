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

#include <bg2e/gpu/vk/ShaderModule.hpp>
#include <fstream>
#include <vector>
#include <stdexcept>

namespace bg2e {
namespace gpu {
namespace vk {

ShaderModule::ShaderModule(VkDevice device, const gpu::ShaderModuleDescription& description)
    : _device(device), _stage(description.stage), _entryPoint(description.entryPoint)
{
    std::ifstream file(description.filePath, std::ios::ate | std::ios::binary);
    if (!file.is_open())
    {
        throw std::runtime_error("vk::ShaderModule: failed to open shader file: " + description.filePath);
    }

    size_t fileSize = static_cast<size_t>(file.tellg());
    if (fileSize == 0 || fileSize % 4 != 0)
    {
        throw std::runtime_error("vk::ShaderModule: invalid SPIR-V file size (must be non-zero and 4-byte aligned): " + description.filePath);
    }

    std::vector<uint32_t> code(fileSize / sizeof(uint32_t));
    file.seekg(0);
    file.read(reinterpret_cast<char*>(code.data()), fileSize);
    file.close();

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size() * sizeof(uint32_t);
    createInfo.pCode = code.data();

    VK_ASSERT(vkCreateShaderModule(_device, &createInfo, nullptr, &_shaderModule));
}

ShaderModule::~ShaderModule()
{
    cleanup();
}

ShaderStage ShaderModule::stage() const
{
    return _stage;
}

bool ShaderModule::isValid() const
{
    return _shaderModule != VK_NULL_HANDLE;
}

void ShaderModule::cleanup()
{
    if (_shaderModule != VK_NULL_HANDLE)
    {
        vkDestroyShaderModule(_device, _shaderModule, nullptr);
        _shaderModule = VK_NULL_HANDLE;
    }
}

}
}
}
