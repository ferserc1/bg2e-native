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

#include <bg2e/gpu/metal/Sampler.hpp>
#include <bg2e/base/Log.hpp>

#include <stdexcept>

namespace bg2e {
namespace gpu {
namespace metal {

#if BG2E_IS_MAC

Sampler::Sampler(gpu::Device* gpuDevice, DeviceHandle device, const gpu::SamplerDescription& description)
    : gpu::Sampler(gpuDevice), _device(device)
{
    auto* desc = MTL::SamplerDescriptor::alloc()->init();

    switch (description.minFilter)
    {
        case gpu::Filter::Nearest: desc->setMinFilter(MTL::SamplerMinMagFilterNearest); break;
        case gpu::Filter::Linear:  desc->setMinFilter(MTL::SamplerMinMagFilterLinear);  break;
    }

    switch (description.magFilter)
    {
        case gpu::Filter::Nearest: desc->setMagFilter(MTL::SamplerMinMagFilterNearest); break;
        case gpu::Filter::Linear:  desc->setMagFilter(MTL::SamplerMinMagFilterLinear);  break;
    }

    switch (description.mipFilter)
    {
        case gpu::MipmapFilter::Nearest: desc->setMipFilter(MTL::SamplerMipFilterNearest); break;
        case gpu::MipmapFilter::Linear:  desc->setMipFilter(MTL::SamplerMipFilterLinear);  break;
    }

    auto toMtlAddressMode = [](gpu::AddressMode m) -> MTL::SamplerAddressMode {
        switch (m)
        {
            case gpu::AddressMode::Repeat:         return MTL::SamplerAddressModeRepeat;
            case gpu::AddressMode::ClampToEdge:    return MTL::SamplerAddressModeClampToEdge;
            case gpu::AddressMode::MirroredRepeat: return MTL::SamplerAddressModeMirrorRepeat;
        }
        return MTL::SamplerAddressModeRepeat;
    };

    desc->setSAddressMode(toMtlAddressMode(description.addressModeU));
    desc->setTAddressMode(toMtlAddressMode(description.addressModeV));
    desc->setRAddressMode(toMtlAddressMode(description.addressModeW));
    desc->setLabel(NS::String::string(description.debugName.c_str(), NS::UTF8StringEncoding));

    _samplerState = _device->newSamplerState(desc);
    desc->release();

    if (!_samplerState)
    {
        throw std::runtime_error("metal::Sampler: newSamplerState failed");
    }
}

Sampler::~Sampler()
{
    cleanup();
}

bool Sampler::isValid() const
{
    return _samplerState != nullptr;
}

void Sampler::cleanup()
{
    if (_samplerState)
    {
        _samplerState->release();
        _samplerState = nullptr;
    }
}

#else

Sampler::Sampler(gpu::Device* gpuDevice, DeviceHandle, const gpu::SamplerDescription&)
    : gpu::Sampler(gpuDevice)
{
    throw std::runtime_error("Metal backend is not available on this platform");
}

Sampler::~Sampler() {}

bool Sampler::isValid() const { return false; }

void Sampler::cleanup() {}

#endif

}
}
}
