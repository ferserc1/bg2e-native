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

#include <bg2e/gpu/ShaderLib.hpp>
#include <bg2e/gpu/Device.hpp>

namespace bg2e {
namespace gpu {

ShaderLib::ShaderLib(const std::filesystem::path& basePath, BackendType backendType)
    : _basePath(basePath)
    , _backendType(backendType)
{
}

std::shared_ptr<ShaderModule> ShaderLib::vertex(const std::string& shaderName, Device* device, const std::string& debugName)
{
    return _load(
        shaderName,
        "vert",
        ShaderStage::Vertex,
        debugName.empty() ?
            shaderName + " vertex shader" :
            debugName,
        device
    );
}

std::shared_ptr<ShaderModule> ShaderLib::fragment(const std::string& shaderName, Device* device, const std::string& debugName)
{
    return _load(
        shaderName,
        "frag",
        ShaderStage::Fragment,
        debugName.empty() ?
            shaderName + " fragment shader" :
            debugName,
        device
    );
}

std::shared_ptr<ShaderModule> ShaderLib::compute(const std::string& shaderName, Device* device, const std::string& debugName)
{
    return _load(
        shaderName,
        "comp",
        ShaderStage::Compute,
        debugName.empty() ?
            shaderName + " compute shader" :
            debugName,
        device
    );
}

std::shared_ptr<ShaderModule> ShaderLib::_load(
    const std::string& shaderName,
    const std::string& stageExt,
    ShaderStage        stage,
    const std::string& debugName,
    Device*            device)
{
    std::string ext        = (_backendType == BackendType::Vulkan) ? ".spv" : ".metallib";
    std::string entryPoint = stageExt + "Main";   // e.g. "vert" -> "vertMain"
    auto filePath = (_basePath / (shaderName + "." + stageExt + ext)).string();
    return device->createShaderModule({ filePath, entryPoint, stage, debugName });
}

}
}
