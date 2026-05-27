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

#include <bg2e/gpu/Instance.hpp>
#include <bg2e/gpu/metal/common.hpp>

#include <string>

namespace bg2e {
namespace gpu {
namespace metal {

class Instance : public gpu::Instance {
public:
    Instance();

    void setApplicationName(const std::string& name) override;
    const std::string& applicationName() const override;
    void enableDebugMode(bool value) override;
    bool debugModeEnabled() const override;

    void create(SDL_Window* window) override;
    void create() override;
    void cleanup() override;

private:
    bool _debugMode = false;
    std::string _applicationName = "bg2 engine Metal Application";

    void assertMetalSupport();
};

}
}
}