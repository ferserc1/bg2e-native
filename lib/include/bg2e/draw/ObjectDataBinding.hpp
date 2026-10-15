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
  private:
    struct Record {
        RenderQueue::Item owner;
        std::shared_ptr<gpu::Buffer> buffer;
        std::shared_ptr<gpu::ResourceSet> set;
    };
    struct Slot {
        std::vector<Record> records;
        uint64_t frameNumber = 0;
        bool prepared = false;
    };
    Engine* _engine = nullptr;
    std::shared_ptr<gpu::PipelineLayout> _layout;
    std::vector<Slot> _slots;
};
}
