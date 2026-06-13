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

#include <bg2e/gpu/metal/ShaderModule.hpp>
#include <bg2e/base/Log.hpp>
#include <stdexcept>

namespace bg2e {
namespace gpu {
namespace metal {

#if BG2E_IS_MAC

ShaderModule::ShaderModule(gpu::Device* gpuDevice, MTL::Device* device, const gpu::ShaderModuleDescription& description)
    : gpu::ShaderModule(gpuDevice), _device(device), _stage(description.stage), _entryPoint(description.entryPoint)
{
    if (!_device)
    {
        throw std::runtime_error("metal::ShaderModule: invalid device");
    }

    // Convert file path to NS::String
    auto* nsPath = NS::String::string(description.filePath.c_str(), NS::UTF8StringEncoding);
    auto* fileURL = NS::URL::fileURLWithPath(nsPath);

    // Load the metallib
    NS::Error* error = nullptr;
    _library = _device->newLibrary(fileURL, &error);
    if (!_library)
    {
        std::string errorMsg = "metal::ShaderModule: failed to load metallib: " + description.filePath;
        if (error)
        {
            errorMsg += " - " + std::string(error->localizedDescription()->utf8String());
        }
        throw std::runtime_error(errorMsg);
    }

    // Get the function by entry point name
    auto* nsEntryPoint = NS::String::string(description.entryPoint.c_str(), NS::UTF8StringEncoding);
    _function = _library->newFunction(nsEntryPoint);
    if (!_function)
    {
        _library->release();
        _library = nullptr;
        throw std::runtime_error("metal::ShaderModule: failed to find function '" + description.entryPoint + "' in " + description.filePath);
    }

    if (base::Log::isDebug() && !description.debugName.empty())
    {
        _library->setLabel(NS::String::string(description.debugName.c_str(), NS::UTF8StringEncoding));
        std::string funcLabel = description.debugName + "::" + description.entryPoint;
        _function->setLabel(NS::String::string(funcLabel.c_str(), NS::UTF8StringEncoding));
    }
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
    return _function != nullptr;
}

void ShaderModule::cleanup()
{
    if (_function)
    {
        _function->release();
        _function = nullptr;
    }
    if (_library)
    {
        _library->release();
        _library = nullptr;
    }
}

#else

ShaderModule::ShaderModule(gpu::Device* gpuDevice, void* /*device*/, const gpu::ShaderModuleDescription& description)
    : gpu::ShaderModule(gpuDevice), _stage(description.stage), _entryPoint(description.entryPoint)
{
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
    return false;
}

void ShaderModule::cleanup()
{
}

#endif

}
}
}
