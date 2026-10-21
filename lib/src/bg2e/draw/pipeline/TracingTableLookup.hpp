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

#include <bg2e/draw/MaterialBase.hpp>
#include <bg2e/gpu/Image.hpp>
#include <bg2e/gpu/Sampler.hpp>
#include <cstddef>
#include <cstdint>
#include <functional>

namespace bg2e::draw::pipeline::detail {
inline size_t combineHash(size_t seed, size_t value)
{
    return seed ^ (value + size_t(0x9e3779b9u) + (seed << 6) + (seed >> 2));
}

// Keys are borrowed only while their snapshot/collection retains the resources.
// Images can be recreated in place; native sampler objects are immutable.
struct ImageKey {
    const gpu::Image* image = nullptr;
    uint64_t generation = 0;
    bool operator==(const ImageKey&) const = default;
};
struct ImageHash {
    size_t operator()(const ImageKey& key) const
    {
        return combineHash(std::hash<const gpu::Image*>{}(key.image), std::hash<uint64_t>{}(key.generation));
    }
};
struct AlphaPairKey {
    ImageKey image;
    const gpu::Sampler* sampler = nullptr;
    bool operator==(const AlphaPairKey&) const = default;
};
struct AlphaPairHash {
    size_t operator()(const AlphaPairKey& key) const
    {
        return combineHash(ImageHash{}(key.image), std::hash<const gpu::Sampler*>{}(key.sampler));
    }
};
struct MaterialKey {
    const MaterialBase* material = nullptr;
    uint64_t values = 0, bindings = 0, classification = 0;
    bool operator==(const MaterialKey&) const = default;
};
struct MaterialHash {
    size_t operator()(const MaterialKey& key) const
    {
        auto result = std::hash<const MaterialBase*>{}(key.material);
        result = combineHash(result, std::hash<uint64_t>{}(key.values));
        result = combineHash(result, std::hash<uint64_t>{}(key.bindings));
        return combineHash(result, std::hash<uint64_t>{}(key.classification));
    }
};
}
