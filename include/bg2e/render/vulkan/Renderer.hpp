
#ifndef bg2e_render_vulkan_renderer_hpp
#define bg2e_render_vulkan_renderer_hpp

#include <bg2e/render/Renderer.hpp>
#include <bg2e/render/CommandQueue.hpp>
#include <bg2e/render/vulkan/VulkanAPI.hpp>

#include <memory>
#include <functional>

namespace bg2e {
namespace render {
namespace vulkan {


class BG2E_EXPORT Renderer : public render::Renderer
{
public:
    Renderer();
    
    virtual void init(const std::string& appName);
    
    virtual void bindWindow(app::Window&);
    
    virtual void resize(uint32_t w, uint32_t h);
    
    virtual void update(float delta);
    
    virtual void drawFrame();
    
    virtual void destroy();
    
    virtual CommandQueue* commandQueue();
    
    inline VulkanAPI* vulkanApi() { return _vulkanApi.get(); }
    
private:
    std::shared_ptr<VulkanAPI> _vulkanApi;
    std::string _appName;
};

}
}
}

#endif

