#pragma once
#include <bg2e/draw/DataBinding.hpp>
#include <bg2e/draw/ShaderData.hpp>

namespace bg2e::draw {
class BG2E_API FrameDataBinding : public DataBinding {
  public:
    void initialize(Engine& engine, std::shared_ptr<gpu::PipelineLayout> layout, uint32_t frameSlots = 0);
    void update(const FrameContext& frame, const glm::mat4& view, const glm::mat4& projection);
};
}
