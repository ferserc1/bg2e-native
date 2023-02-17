//
//  Window.hpp
//  bg2e
//
//  Created by Fernando Serrano Carpena on 17/2/23.
//

#ifndef bg2e_app_window_hpp
#define bg2e_app_window_hpp

#include <string>

namespace bg2e {
namespace app {

class Window {
public:
    Window(const std::string & title = "", uint32_t width = 800, uint32_t height = 600);
    
    inline void setTitle(const std::string & title) { _title = title; }
    inline const std::string& title() const { return _title; }
    inline void setSize(uint32_t width, uint32_t height) { _width = width; height = _height; }
    inline uint32_t width() const { return _width; }
    inline uint32_t height() const { return _height; }
    
    void create();
    void destroy();
    
    void* impl_ptr() { return _wnd; }
    
    inline bool created() const { return _wnd != nullptr; }
    
protected:
    void* _wnd = nullptr;
    
    std::string _title = "";
    uint32_t _width = 800;
    uint32_t _height = 600;
};


}
}

#endif /* Window_h */
