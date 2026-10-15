#pragma once
#include <bg2e/draw/DataBinding.hpp>
#include <bg2e/draw/ShaderData.hpp>
#include <bg2e/draw/environment/EnvironmentResources.hpp>

namespace bg2e::draw {
class BG2E_API EnvironmentDataBinding : public DataBinding {
  public:
    void initialize(Engine& engine, std::shared_ptr<gpu::PipelineLayout> layout, uint32_t frameSlots = 0, uint32_t metalTextureBase = 6);
    void update(const FrameContext& frame, std::shared_ptr<const environment::EnvironmentResources> environment);
    void cleanup() override;
  private:
    uint32_t _metalTextureBase = 6;
    std::vector<std::shared_ptr<const environment::EnvironmentResources>> _owners;
};
}
