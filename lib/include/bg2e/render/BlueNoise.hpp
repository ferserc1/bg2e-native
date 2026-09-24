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

#include <glm/glm.hpp>

namespace bg2e::render {

/*
 *  Shared blue-noise texture for stochastic sampling (RTAO, RTGI,
 *  RT reflections). The texture data is precomputed and embedded in
 *  blue_noise_data.h (next to the .cpp), which is self-descriptive:
 *  it defines the size, channel count and layer count, so replacing
 *  that file is enough to swap the noise data.
 *
 *  The image is created as a 2D array with one layer per precomputed
 *  tile, so shaders always sample it as sampler2DArray regardless of
 *  the layer count.
 */
class BG2E_API BlueNoise {
public:
    explicit BlueNoise(Engine* engine);
    ~BlueNoise();

    void build();
    void cleanup();

    VkImageView imageView() const { return _imageView; }
    VkSampler sampler() const { return _sampler; }
    glm::uvec2 size() const { return _size; }
    uint32_t layerCount() const { return _layerCount; }

private:
    Engine* _engine = nullptr;

    VkImage _image = VK_NULL_HANDLE;
    VkImageView _imageView = VK_NULL_HANDLE;
    VmaAllocation _allocation = VK_NULL_HANDLE;
    VkSampler _sampler = VK_NULL_HANDLE;

    glm::uvec2 _size = glm::uvec2(0);
    uint32_t _layerCount = 0;
};

}
