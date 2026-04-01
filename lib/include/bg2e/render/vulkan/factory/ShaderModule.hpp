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

#pragma once

#include <bg2e/common.hpp>
#include <bg2e/render/Engine.hpp>

#include <string>

namespace bg2e {
namespace render {
namespace vulkan {
namespace factory {

class BG2E_API ShaderModule
{
public:
    // If the basePath is not specified, the shader will be loaded from the
    // default shader path (see PlatformTools::shaderPath())
    static VkShaderModule loadFromSPV(
        const std::string& fileName,
        VkDevice device,
        const std::string& basePath = ""
    );
};

}
}
}
}

