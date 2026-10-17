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

#include <stdexcept>

namespace bg2e {
namespace gpu {

ShaderLib::ShaderLib(const std::filesystem::path& basePath, BackendType backendType)
    : _basePath(basePath), _backendType(backendType)
{
}

namespace {
std::string stageExtension(ShaderStage stage)
{
    switch (stage)
    {
    case ShaderStage::Vertex: return "vert";
    case ShaderStage::Fragment: return "frag";
    case ShaderStage::Compute: return "comp";
    case ShaderStage::RayGeneration: return "rgen";
    case ShaderStage::Miss: return "rmiss";
    case ShaderStage::ClosestHit: return "rchit";
    case ShaderStage::AnyHit: return "rahit";
    default: throw std::invalid_argument("ShaderLib requires one supported shader stage");
    }
}
}

std::filesystem::path ShaderLib::stagePath(const std::string& shaderName, ShaderStage stage) const
{
    const std::filesystem::path name(shaderName);
    if (name.empty() || name.is_absolute() || name.has_root_path() || name.filename().empty())
    {
        throw std::invalid_argument("ShaderLib requires a relative shader name");
    }
    for (const auto& part : name)
    {
        if (part == ".." || part == ".")
        {
            throw std::invalid_argument("ShaderLib shader names cannot traverse directories");
        }
    }
    const auto extension = _backendType == BackendType::Vulkan ? ".spv" : ".metallib";
    return _basePath / (name.generic_string() + "." + stageExtension(stage) + extension);
}

std::string ShaderLib::entryPoint(BackendType backendType, ShaderStage stage)
{
    const auto extension = stageExtension(stage);
    return backendType == BackendType::Vulkan ? "main" : extension + "Main";
}

std::shared_ptr<ShaderModule> ShaderLib::vertex(const std::string& shaderName, Device* device, const std::string& debugName)
{
    return _load(
        shaderName,
        ShaderStage::Vertex,
        debugName.empty() ? shaderName + " vertex shader" : debugName,
        device);
}

std::shared_ptr<ShaderModule> ShaderLib::fragment(const std::string& shaderName, Device* device, const std::string& debugName)
{
    return _load(
        shaderName,
        ShaderStage::Fragment,
        debugName.empty() ? shaderName + " fragment shader" : debugName,
        device);
}

std::shared_ptr<ShaderModule> ShaderLib::compute(const std::string& shaderName, Device* device, const std::string& debugName)
{
    return _load(
        shaderName,
        ShaderStage::Compute,
        debugName.empty() ? shaderName + " compute shader" : debugName,
        device);
}

std::shared_ptr<ShaderModule> ShaderLib::rayGeneration(const std::string& shaderName, Device* device, const std::string& debugName)
{
    return _load(
        shaderName,
        ShaderStage::RayGeneration,
        debugName.empty() ? shaderName + " ray generation shader" : debugName,
        device);
}

std::shared_ptr<ShaderModule> ShaderLib::miss(const std::string& shaderName, Device* device, const std::string& debugName)
{
    return _loadOrNull(
        shaderName,
        ShaderStage::Miss,
        debugName.empty() ? shaderName + " miss shader" : debugName,
        device);
}

std::shared_ptr<ShaderModule> ShaderLib::closestHit(const std::string& shaderName, Device* device, const std::string& debugName)
{
    return _loadOrNull(
        shaderName,
        ShaderStage::ClosestHit,
        debugName.empty() ? shaderName + " closest hit shader" : debugName,
        device);
}

std::shared_ptr<ShaderModule> ShaderLib::anyHit(const std::string& shaderName, Device* device, const std::string& debugName)
{
    return _loadOrNull(shaderName, ShaderStage::AnyHit,
        debugName.empty() ? shaderName + " any hit shader" : debugName, device);
}

std::shared_ptr<ShaderModule> ShaderLib::_load(
    const std::string& shaderName,
    ShaderStage stage,
    const std::string& debugName,
    Device* device)
{
    if (!device)
    {
        throw std::invalid_argument("ShaderLib requires a device");
    }
    const auto filePath = stagePath(shaderName, stage);
    return device->createShaderModule({filePath.string(), entryPoint(_backendType, stage), stage, debugName});
}

std::shared_ptr<ShaderModule> ShaderLib::_loadOrNull(
    const std::string& shaderName,
    ShaderStage stage,
    const std::string& debugName,
    Device* device)
{
    const auto filePath = stagePath(shaderName, stage);
    if (_backendType == BackendType::Metal && !std::filesystem::exists(filePath))
    {
        return nullptr;
    }
    return _load(shaderName, stage, debugName, device);
}

}
}
