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

#include <bg2e/gpu/Device.hpp>
#include <bg2e/base/CpuProfile.hpp>

namespace bg2e::gpu {

Device::ResourceUpdateScope::ResourceUpdateScope(Device& device) : _device(device)
{
    _device._submissions->beginResourceUpdate();
    try
    {
        _device.waitIdleForResourceUpdate();
    }
    catch (...)
    {
        _device._submissions->endResourceUpdate();
        throw;
    }
}


Device::ResourceUpdateScope::~ResourceUpdateScope()
{
    _device._submissions->endResourceUpdate();
}


bool Device::waitIdleForResourceUpdate()
{
    base::CpuProfileScope::count("gpu.resourceUpdateSyncRequests", 1);
    if (_submissions->resourceUpdateIdle())
    {
        base::CpuProfileScope::count("gpu.resourceUpdateWaitsReused", 1);
        return false;
    }
    base::CpuProfileScope profile("gpu.waitIdle.resourceUpdate");
    base::CpuProfileScope::count("gpu.resourceUpdateDrains", 1, "drains");
    waitIdle();
    return true;
}

}
