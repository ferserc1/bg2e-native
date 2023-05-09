#ifndef bg2e_render_vulkan_pipeline_hpp
#define bg2e_render_vulkan_pipeline_hpp

#include <bg2e/export.hpp>
#include <bg2e/render/vulkan/PipelineLayout.hpp>

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
    
    void build();
    
protected:
    std::shared_ptr<PipelineLayout> _pipelineLayout;
};

}
}
}

#endif

