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

#include <bg2e/gpu/vk/Backend.hpp>
#include <bg2e/gpu/vk/common.hpp>

namespace bg2e {
namespace gpu {
namespace vk {

void Backend::init(SDL_Window* window) {}
void Backend::setEngine(void* engine) {}
void Backend::cleanup() {}

Buffer* Backend::createBuffer(size_t size, uint32_t usageFlags, uint32_t memoryUsage) { return nullptr; }
void Backend::destroyBuffer(Buffer* buffer) {}

Image* Backend::createImage(uint32_t format, uint32_t width, uint32_t height,
                            uint32_t usage, uint32_t aspectFlags,
                            uint32_t arrayLayers, bool useMipmaps,
                            uint32_t maxMipmapLevels, uint32_t samples) { return nullptr; }
Image* Backend::createImageFromData(const uint8_t* data, size_t dataSize,
                                    uint32_t width, uint32_t height, uint32_t bpp,
                                    uint32_t format, uint32_t usage) { return nullptr; }
void Backend::destroyImage(gpu::Image* image) {}

bool Backend::rayQuerySupported() const { return false; }
bool Backend::rayTracingPipelineSupported() const { return false; }
bool Backend::newFrame() { return false; }

}
}
}
