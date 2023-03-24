//
//  Window.hpp
//  bg2e
//
//  Created by Fernando Serrano Carpena on 17/2/23.
//

#ifndef bg2e_app_window_hpp
#define bg2e_app_window_hpp

#include <bg2e/export.hpp>
#include <bg2e/app/AppController.hpp>
#include <bg2e/render/Renderer.hpp>

#include <string>
#include <memory>

namespace bg2e {
namespace app {


class BG2E_EXPORT Window {
public:
    Window(const std::string & title = "", uint32_t width = 800, uint32_t height = 600);
    
    void setRenderer(std::unique_ptr<render::Renderer>&&);
    
    inline render::Renderer& renderer() { return *(_renderer.get()); }
    
    inline void setAppController(std::unique_ptr<AppController> ctrl) {
        _appController = std::move(ctrl);
        _appController->_window = this;
    }
    
    inline AppController* appController() { return _appController.get(); }
    
    void setTitle(const std::string & title);
    inline const std::string& title() const { return _title; }
    inline void setSize(uint32_t width, uint32_t height) { _width = width; height = _height; }
    inline uint32_t width() const { return _width; }
    inline uint32_t height() const { return _height; }
    
    void create();
    void destroy();
    
    void* impl_ptr() { return _wnd; }
    
    inline bool created() const { return _wnd != nullptr; }
    
private:
    void* _wnd = nullptr;
    
    std::string _title = "";
    uint32_t _width = 800;
    uint32_t _height = 600;
    
    std::unique_ptr<AppController> _appController;
    std::unique_ptr<render::Renderer> _renderer;
};


}
}

#endif /* Window_h */
