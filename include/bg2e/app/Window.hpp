//
//  Window.hpp
//  bg2e
//
//  Created by Fernando Serrano Carpena on 17/2/23.
//

#ifndef bg2e_app_window_hpp
#define bg2e_app_window_hpp

#include <bg2e/export.hpp>
#include <bg2e/types.hpp>

#include <bg2e/app/AppController.hpp>
#include <bg2e/render/Renderer.hpp>

#include <string>
#include <memory>

namespace bg2e {
namespace app {


class BG2E_EXPORT Window {
public:
    Window(const std::string & title = "", uint32_t width = 800, uint32_t height = 600);
    Window(const std::string& title, const Size& size) :Window(title, size.width, size.height) {}
    Window(const std::string& title, Size&& size) :Window(title, size.width, size.height) {}
    
    void setRenderer(std::unique_ptr<render::Renderer>&&);
    
    inline render::Renderer& renderer() { return *(_renderer.get()); }
    
    inline void setAppController(std::unique_ptr<AppController> ctrl) {
        _appController = std::move(ctrl);
        _appController->_window = this;
    }
    
    inline AppController* appController() { return _appController.get(); }
    
    void setTitle(const std::string & title);
    inline const std::string& title() const { return _title; }
    inline void setSize(const Size& size) { _size = size; }
    inline void setSize(Size&& size) { _size = size; }
    inline const Size& size() const { return _size; }
    
    void create();
    void destroy();
    
    void* impl_ptr() { return _wnd; }
    
    inline bool created() const { return _wnd != nullptr; }
    
private:
    void* _wnd = nullptr;
    
    std::string _title = "";
    Size _size{800, 600};
    
    std::unique_ptr<AppController> _appController;
    std::unique_ptr<render::Renderer> _renderer;
};


}
}

#endif /* Window_h */
