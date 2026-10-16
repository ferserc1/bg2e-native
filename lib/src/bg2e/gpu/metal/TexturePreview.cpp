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

#include <bg2e/gpu/metal/TexturePreview.hpp>
#include <bg2e/base/PlatformTools.hpp>
#ifdef BG2E_IS_MAC
#include <bg2e/gpu/metal/Image.hpp>
#include <stdexcept>

namespace bg2e::gpu::metal {

uint64_t registerTexturePreview(gpu::Image& image, gpu::Sampler& sampler)
{
    auto* native = dynamic_cast<metal::Image*>(&image);
    if (!native || !native->texture() || image.device() != sampler.device())
    {
        throw std::invalid_argument("Preview requires a Metal image and same-device sampler");
    }
    // ImGui Metal owns its linear clamp sampler; the image owner is retained by UI.
    return reinterpret_cast<uint64_t>(native->texture());
}

void removeTexturePreview(uint64_t)
{
}

}
#else
#include <stdexcept>

namespace bg2e::gpu::metal {

uint64_t registerTexturePreview(gpu::Image&, gpu::Sampler&)
{
    throw std::runtime_error("Metal texture previews are unavailable on this platform");
}

void removeTexturePreview(uint64_t)
{
}

}
#endif
