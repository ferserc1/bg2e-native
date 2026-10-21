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
#include <bg2e/draw/DataBinding.hpp>
#include <bg2e/draw/RenderQueue.hpp>

namespace bg2e::draw {
// Six textures are ordered Albedo/Metalness/Roughness/Normal/AO/Emission.
// Slots retain queue/resource snapshots; geometry and textures are not rings.
class BG2E_API ObjectDataBinding {
  public:
    ObjectDataBinding() = default;
    ObjectDataBinding(const ObjectDataBinding&) = delete;
    ObjectDataBinding& operator=(const ObjectDataBinding&) = delete;
    void initialize(Engine& engine, std::shared_ptr<gpu::PipelineLayout> layout, uint32_t frameSlots = 0);
    void prepare(const FrameContext& frame, const RenderQueue& queue);
    gpu::ResourceSet* set(const FrameContext& frame, uint32_t objectId) const;
    void cleanup();

    void setReuseEnabled(bool enabled)
    {
        _reuseEnabled = enabled;
    }

  private:
    struct Record {
        RenderQueue::Item owner;
        std::shared_ptr<gpu::Buffer> buffer;
        std::shared_ptr<gpu::ResourceSet> set;
        ObjectData data;
        std::array<std::shared_ptr<gpu::Image>, 6> images;
        std::array<std::shared_ptr<gpu::Sampler>, 6> samplers;
        bool valid = false;
    };
    struct Slot {
        std::vector<Record> records;
        uint64_t frameNumber = 0;
        bool prepared = false;
    };
    Engine* _engine = nullptr;
    std::shared_ptr<gpu::PipelineLayout> _layout;
    std::vector<Slot> _slots;
    bool _reuseEnabled = true;
};
}
