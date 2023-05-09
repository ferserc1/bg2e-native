#ifndef bg2e_render_vulkan_pipeline_hpp
#define bg2e_render_vulkan_pipeline_hpp

#include <bg2e/export.hpp>
#include <bg2e/render/vulkan/PipelineLayout.hpp>
#include <bg2e/render/vulkan/VulkanAPI.hpp>

#include <memory>

namespace bg2e {
namespace render {
namespace vulkan {

class BG2E_EXPORT Pipeline {
public:
    Pipeline();
    Pipeline(const Pipeline*);
    
    inline void setLayout(std::shared_ptr<PipelineLayout> layout) {
        _pipelineLayout = layout;
    }
    
    void build(VulkanAPI *);

    void destroy();

    vk::Pipeline impl() const { return _pipeline;  }
    
protected:
    std::shared_ptr<PipelineLayout> _pipelineLayout;

    vk::GraphicsPipelineCreateInfo _createInfo;
    vk::Pipeline _pipeline;
    vk::Device _device;
};

}
}
}

#endif

