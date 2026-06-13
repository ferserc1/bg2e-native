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

#include <bg2e/gpu/ResourceSet.hpp>
#include <bg2e/gpu/metal/common.hpp>

#include <vector>

namespace bg2e {
namespace gpu {
namespace metal {

class PipelineLayout;

struct ResourceEntry {
    uint32_t             index = 0;    // Metal arg index (from PipelineLayout)
    ResourceType         type = ResourceType::SampledImage;
    ShaderStage          stage = ShaderStage::Fragment;
    TextureHandle        texture = nullptr;    // for Sampled/Storage image
    SampleStateHandle    sampler = nullptr;    // for Sampler
};

class BG2E_API ResourceSet : public gpu::ResourceSet {
public:
    ResourceSet(const metal::PipelineLayout* layout, uint32_t setIndex);
    ~ResourceSet() override;

    void setStorageImage(uint32_t binding, gpu::Image* image) override;
    void setSampledImage(uint32_t binding, gpu::Image* image) override;
    void setSampler(uint32_t binding, gpu::Sampler* sampler) override;

    void update() override;

    uint32_t setIndex() const override { return _setIndex; }
    bool isValid() const override;
    void cleanup() override;

    const std::vector<ResourceEntry>& entries() const { return _entries; }

private:
    uint32_t    _setIndex{0};
    const metal::PipelineLayout* _layout = nullptr;
    std::vector<ResourceEntry>   _entries;
};

}
}
}
