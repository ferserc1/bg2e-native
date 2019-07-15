

#ifndef _bg2_wnd_window_hpp_
#define _bg2_wnd_window_hpp_

#include <bg2math/vector.hpp>

#include <string>

namespace bg2wnd {
    
    class Window {
    public:
        static Window * Create();
        
        inline void setSize(const bg2math::int2 & size) { _size = size; }
        inline void setTitle(const std::string & title) { _title = title; }
        inline const bg2math::int2 & size() const { return _size; }
        inline const std::string & title() const { return _title; }
        
        void build();
        
        bool shouldClose();
        
        virtual ~Window();
        
    protected:
        Window();
        
        void * _windowPtr = nullptr;
        bg2math::int2 _size = bg2math::int2(800,600);
        std::string _title = "Window";
    };
}
#endif
