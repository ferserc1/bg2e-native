/*
 *    business grade graphic engine (bg2e engine)
 *    Copyright (C) 2026  Fernando Serrano Carpena
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of GNU General Public License as published by
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

#include <bg2e/gpu/Factory.hpp>

namespace bg2e {
namespace gpu {

BackendType Factory::_defaultType = BackendType::Vulkan;

std::unique_ptr<Backend> Factory::create(BackendType type) {
    return nullptr;
}

void Factory::setDefault(BackendType type) {
    _defaultType = type;
}

Backend& Factory::defaultBackend() {
    static Backend* instance = nullptr;
    if (!instance) {
        instance = &defaultBackend();
    }
    return *instance;
}

}
}
