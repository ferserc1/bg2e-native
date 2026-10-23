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

#include <bg2e/gpu/metal/MetalFXBridge.hpp>

#ifndef __APPLE__
namespace bg2e::gpu::metal {
struct MetalFXBridge::State {};

MetalFXBridge::MetalFXBridge() = default;

MetalFXBridge::~MetalFXBridge() = default;

bool MetalFXBridge::sdkAvailable()
{
    return false;
}

bool MetalFXBridge::supportsDevice(const gpu::Device&)
{
    return false;
}

bool MetalFXBridge::initialize(gpu::Device&, Size2D, Size2D, uint32_t)
{
    _status = "MetalFX requires macOS 13 or later and a supported Metal device; bilinear reconstruction active";
    return false;
}

void MetalFXBridge::cleanup()
{
    _state.reset();
    _status = "Not initialized";
}

bool MetalFXBridge::available() const
{
    return false;
}

const std::string& MetalFXBridge::status() const
{
    return _status;
}

bool MetalFXBridge::record(const MetalFXDispatch&)
{
    return false;
}
}
#endif
