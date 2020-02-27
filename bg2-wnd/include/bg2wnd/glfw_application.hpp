
#ifndef _bg2_wnd_glfw_application_hpp_
#define _bg2_wnd_glfw_application_hpp_

#include <bg2wnd/application.hpp>
#include <bg2base/platform.hpp>

namespace bg2wnd {
    
    class GlfwApplication : public Application {
    public:
        virtual void build() override;
        
        virtual int run() override;
        
        virtual Window * getWindow(bg2base::plain_ptr nativeWindowHandler) override;
    };
}

#endif

