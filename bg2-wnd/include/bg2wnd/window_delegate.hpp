
#ifndef _bg2_wnd_window_delegate_hpp_
#define _bg2_wnd_window_delegate_hpp_

#include <bg2math/vector.hpp>
#include <bg2wnd/keyboard_event.hpp>
#include <bg2wnd/mouse_event.hpp>

namespace bg2wnd {

    class WindowDelegate {
    public:
        virtual ~WindowDelegate() {}
        
        virtual void init() {}
        virtual void resize(const bg2math::int2 & size) {}
        virtual void update(float delta) {}
        virtual void draw() {}
        virtual void cleanup() {}
        
        // TODO: implement events
        virtual void keyDown(const KeyboardEvent &) {}
        virtual void keyUp(const KeyboardEvent &) {}
    
        
    };
    
}

#endif
