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

#include <bg2e/common.hpp>
#include <bg2e/gpu/Common.hpp>

#include <cstdint>

namespace bg2e {
namespace gpu {

class Image;
class Sampler;

class BG2E_API ResourceSet {
public:
    virtual ~ResourceSet() = default;

    virtual void setStorageImage(uint32_t binding, gpu::Image* image) = 0;
    virtual void setSampledImage(uint32_t binding, gpu::Image* image) = 0;
    virtual void setSampler(uint32_t binding, gpu::Sampler* sampler)  = 0;
    // Reserved for future iterations (declare but may be left unimplemented
    // / throwing in the backends until needed):
    // virtual void setUniformBuffer(uint32_t binding, gpu::Buffer* buffer) = 0;

    virtual void update() = 0;   // flush assignments to the backend

    virtual uint32_t setIndex() const = 0;
    virtual bool isValid() const = 0;
    virtual void cleanup() = 0;
};

}
}
