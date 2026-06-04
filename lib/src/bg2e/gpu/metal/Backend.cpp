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

#include <bg2e/gpu/metal/Backend.hpp>
#include <bg2e/gpu/metal/PhysicalDevice.hpp>
#include <bg2e/gpu/metal/Device.hpp>
#include <bg2e/gpu/metal/WindowSurface.hpp>
#include <bg2e/gpu/metal/OffscreenSurface.hpp>

namespace bg2e::gpu::metal {

gpu::Instance* Backend::instance() const
{
    if (_instance == nullptr)
    {
        _instance = std::make_unique<gpu::metal::Instance>();
    }
    return _instance.get();
}

gpu::WindowType Backend::windowType() const
{
    return gpu::WindowType::Metal;
}

std::unique_ptr<gpu::PhysicalDevice> Backend::createPhysicalDevice() const
{
    return std::make_unique<metal::PhysicalDevice>();
}

std::unique_ptr<gpu::Device> Backend::createDevice() const
{
    return std::make_unique<metal::Device>();
}

std::unique_ptr<gpu::WindowSurface> Backend::createWindowSurface() const
{
    return std::make_unique<metal::WindowSurface>();
}

std::unique_ptr<gpu::OffscreenSurface> Backend::createOffscreenSurface(uint32_t width, uint32_t height) const
{
    return std::make_unique<metal::OffscreenSurface>(width, height);
}

}