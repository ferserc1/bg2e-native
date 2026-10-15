#pragma once

#include <bg2e/draw/FrameContext.hpp>
#include <bg2e/gpu/Buffer.hpp>
#include <bg2e/gpu/ResourceSet.hpp>
#include <bg2e/gpu/PipelineLayout.hpp>
#include <memory>
#include <vector>

namespace bg2e::draw {
// Full draw ABI: sets 0 frame, 1 object/material, 2 lights, 3 environment.
BG2E_API gpu::PipelineLayoutDescription drawBindingLayout();
// Per-slot mutable data. update is legal exactly once per presentation number
// after acquiring/waiting that frame slot, before recording any consumer.
class BG2E_API DataBinding {
  public:
    virtual ~DataBinding() = default;
    DataBinding() = default;
    DataBinding(const DataBinding&) = delete;
    DataBinding& operator=(const DataBinding&) = delete;
    gpu::ResourceSet* set(const FrameContext& frame) const;
    virtual void cleanup(); // Stop producers first; waits before retiring slots.
  protected:
    struct Slot {
        std::shared_ptr<gpu::Buffer> buffer;
        std::shared_ptr<gpu::ResourceSet> set;
        uint64_t frameNumber = 0;
        bool prepared = false;
    };
    void initializeUniforms(Engine& engine, std::shared_ptr<gpu::PipelineLayout> layout,
        uint32_t setIndex, size_t byteSize, const std::vector<gpu::ShaderBinding>& bindings, uint32_t frameSlots);
    Slot& updateBytes(const FrameContext& frame, const void* data, size_t byteSize);
    Engine* _engine = nullptr;
    std::shared_ptr<gpu::PipelineLayout> _layout;
    std::vector<Slot> _slots;
    size_t _byteSize = 0;
};
}
