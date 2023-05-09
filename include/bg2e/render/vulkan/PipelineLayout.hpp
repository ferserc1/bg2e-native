#ifndef bg2e_render_vulkan_pipelinelayout_hpp
#define bg2e_render_vulkan_pipelinelayout_hpp

#include <bg2e/export.hpp>
#include <bg2e/render/vulkan/VulkanAPI.hpp>

namespace bg2e {
namespace render {
namespace vulkan {

class BG2E_EXPORT PipelineLayout {
public:
    PipelineLayout();

    void build(VulkanAPI*);

    void destroy();

    vk::PipelineLayout impl() const { return _impl; }

protected:
    vk::PipelineLayout _impl;
    vk::Device _device;
};

}
}
}

#endif

