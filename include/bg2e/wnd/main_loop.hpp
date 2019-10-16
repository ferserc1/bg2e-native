#ifndef _bg2e_wnd_main_loop_hpp_
#define _bg2e_wnd_main_loop_hpp_

#include <bg2e/wnd/window.hpp>

#include <vector>

namespace bg2e {
    namespace wnd {

        class MainLoop {
        public:

            MainLoop();
            virtual ~MainLoop();
            
            // registerWindow(): the window will be released by the MainLoop
            void registerWindow(Window *);
            
            inline const std::vector<Window*> & registeredWindows() const { return _windows; }
            
            
            int run();
            
        protected:
            std::vector<Window*> _windows;
        };

    }
} 
#endif
