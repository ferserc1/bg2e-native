
#ifndef _bg2_wnd_application_hpp_
#define _bg2_wnd_application_hpp_

#include <vector>
#include <memory>

#include <bg2math/vector.hpp>
#include <bg2wnd/window.hpp>

namespace bg2wnd {
    
    class Application {
    public:
        virtual ~Application();

        static Application * Create();
       
        void addWindow(Window * wnd);
        
        virtual void build() = 0;

        virtual int run() = 0;
        
    protected:

        Application();

        std::vector<std::shared_ptr<Window>> _windows;
    
    };
}

#endif
