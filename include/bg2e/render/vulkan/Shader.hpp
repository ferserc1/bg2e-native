#ifndef bg2e_render_vulkan_shader_hpp
#define bg2e_render_vulkan_shader_hpp

#include <bg2e/render/vulkan/VulkanApi.hpp>
#include <bg2e/render/vulkan/PipelineLayout.hpp>

namespace bg2e {
namespace render {
namespace vulkan {

class Shader {
public:
    Shader();
    ~Shader();

    void build(VulkanAPI&);

    // TODO: unify shader modules and layout
    std::shared_ptr<PipelineLayout> layout() 
    {
        return _layout;
    }

protected:
    std::shared_ptr<PipelineLayout> _layout;
};

}
}
}

#endif
