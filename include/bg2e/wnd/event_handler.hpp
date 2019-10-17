#ifndef _bg2e_wnd_event_handler_hpp_
#define _bg2e_wnd_event_handler_hpp_

#include <bg2e/wnd/keyboard_event.hpp>
#include <bg2e/wnd/mouse_event.hpp>

namespace bg2e {
    namespace wnd {

        class Window;
    
        class EventHandler {
            friend class Window;
        public:
            virtual void init() {};
            virtual void resize(uint32_t width, uint32_t height) {}
            virtual void update(float delta) {}
            virtual void draw() {}
            virtual void destroy() {}
            virtual void keyDown(const KeyboardEvent &) {}
            virtual void keyUp(const KeyboardEvent &) {}
            virtual void keyRepeat(const KeyboardEvent &) {}
            virtual void mouseDown(const MouseEvent &) {}
            virtual void mouseUp(const MouseEvent &) {}
            virtual void mouseMove(const MouseEvent &) {}
            virtual void mouseDrag(const MouseEvent &) {}
            virtual void mouseWheel(const MouseEvent &) {}
            
            
            inline Window * window() { return _window; }
            inline const Window * window() const { return _window; }
            
        protected:
            Window * _window;
        };
    
    }
}
#endif
