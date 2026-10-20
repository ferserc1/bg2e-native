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

#include <bg2e/gpu/Common.hpp>
#include <bg2e/gpu/ShaderModule.hpp>

#include <filesystem>
#include <memory>
#include <string>

namespace bg2e {
namespace gpu {

class Device;

// Backend-agnostic shader library loader. Resolves per-stage shader files from
// a compiled ShaderLib directory (one .spv or .metallib per shader stage), all
// named <shader>.<stage>.<ext>. Vulkan uses main; Metal uses <stage>Main.
// basePath may select a deployed pipeline folder (e.g. shaders/draw/composite).
class BG2E_API ShaderLib {
  public:
    ShaderLib(const std::filesystem::path& basePath, BackendType backendType);

    // Resolves a file without opening it or creating a GPU resource. Relative nested
    // shader names preserve pipeline directories; absolute/traversing names fail.
    std::filesystem::path stagePath(const std::string& shaderName, ShaderStage stage) const;
    static std::string entryPoint(BackendType backendType, ShaderStage stage);

    std::shared_ptr<ShaderModule> vertex(
        const std::string& shaderName,
        Device* device,
        const std::string& debugName = "");

    std::shared_ptr<ShaderModule> fragment(
        const std::string& shaderName,
        Device* device,
        const std::string& debugName = "");

    std::shared_ptr<ShaderModule> compute(
        const std::string& shaderName,
        Device* device,
        const std::string& debugName = "");

    std::shared_ptr<ShaderModule> rayGeneration(
        const std::string& shaderName,
        Device* device,
        const std::string& debugName = "");

    std::shared_ptr<ShaderModule> miss(
        const std::string& shaderName,
        Device* device,
        const std::string& debugName = "");

    std::shared_ptr<ShaderModule> closestHit(
        const std::string& shaderName,
        Device* device,
        const std::string& debugName = "");

    std::shared_ptr<ShaderModule> anyHit(
        const std::string& shaderName, Device* device, const std::string& debugName = "");

    std::shared_ptr<ShaderModule> specialized(const std::string& shaderName,
        ShaderStage stage, Device* device, const std::vector<ShaderConstant>& constants);

  private:
    std::filesystem::path _basePath;
    BackendType _backendType;

    std::shared_ptr<ShaderModule> _load(
        const std::string& shaderName,
        ShaderStage stage,
        const std::string& debugName,
        Device* device);

    std::shared_ptr<ShaderModule> _loadOrNull(
        const std::string& shaderName,
        ShaderStage stage,
        const std::string& debugName,
        Device* device);
};

}
}
