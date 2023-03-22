
#ifndef bg2e_render_vulkan_renderer_hpp
#define bg2e_render_vulkan_renderer_hpp

#include <bg2e/render/Renderer.hpp>
#include <bg2e/render/vulkan/VulkanAPI.hpp>

#include <memory>

namespace bg2e {
namespace render {
namespace vulkan {


class BG2E_EXPORT Renderer : public render::Renderer
{
public:
    Renderer();
    
    virtual void init(const std::string& appName);
    
    virtual void bindWindow(app::Window&);
    
    virtual void destroy();
    
private:
    std::unique_ptr<VulkanAPI> _vulkanApi;
    std::string _appName;
};

}
}
}

#endif

