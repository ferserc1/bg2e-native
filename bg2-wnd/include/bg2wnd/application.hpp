
#ifndef _bg2_wnd_application_hpp_
#define _bg2_wnd_application_hpp_

#include <vector>
#include <memory>

#include <bg2base/platform.hpp>
#include <bg2math/vector.hpp>
#include <bg2wnd/window.hpp>

namespace bg2wnd {
    
    class Application {
    public:
        virtual ~Application();

        static Application * Get();
       
        void addWindow(Window * wnd);
        
        virtual void build() = 0;

        virtual int run() = 0;

        virtual Window * getWindow(bg2base::plain_ptr nativeWindowHandler) = 0;
        template <class T>
        T * getWindow(bg2base::plain_ptr nativeWindowHandler) { return dynamic_cast<T*>(getWindow(nativeWindowHandler)); }
        
    protected:

        Application();

        std::vector<std::shared_ptr<Window>> _windows;
    
        static Application * s_Application;

        // This function must to be called only from the run() implementation of each
        // platform.
        static void Destroy();
    };
}

#endif
