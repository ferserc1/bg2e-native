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

#include <bg2e/gpu/ShaderModule.hpp>
#include <bg2e/gpu/metal/common.hpp>

#include <string>

namespace bg2e {
namespace gpu {
namespace metal {

class BG2E_API ShaderModule : public gpu::ShaderModule {
public:
#if BG2E_IS_MAC
    ShaderModule(gpu::Device* gpuDevice, MTL::Device* device, const gpu::ShaderModuleDescription& description);
#else
    ShaderModule(gpu::Device* gpuDevice, void* device, const gpu::ShaderModuleDescription& description);
#endif
    ~ShaderModule() override;

    ShaderStage stage() const override;
    bool isValid() const override;
    void cleanup() override;

#if BG2E_IS_MAC
    MTL::Function* function() const { return _function; }
    MTL::Library* library() const { return _library; }
#endif

private:
    ShaderStage _stage;
    std::string _entryPoint;

#if BG2E_IS_MAC
    MTL::Device* _device = nullptr;
    MTL::Library* _library = nullptr;
    MTL::Function* _function = nullptr;
#endif
};

}
}
}
