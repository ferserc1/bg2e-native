#pragma once

#include <bg2e/common.hpp>

#include <functional>
#include <string>
#include <limits>

namespace bg2e {
namespace ui {

class BG2E_API Window {
public:
    enum DockingSide {
        DockLeft,
        DockRight,
        DockBottom
    };
    
	struct Options
	{
		bool noTitleBar = false;
		bool noScrollbar = false;
		bool noMenu = false;
		bool noMove = false;
		bool noResize = false;
		bool noCollapse = false;
		bool noNav = false;
		bool noBackground = false;
		bool noBringToFront = false;
		bool noClose = false;
        int minWidth = 0;
        int minHeight = 0;
        int maxWidth = std::numeric_limits<int>::max();
        int maxHeight = std::numeric_limits<int>::max();
	};

    Options options;

    inline void setTitle(const std::string & title) { _title = title; }
    inline const std::string & title() const { return _title; }

    // Option 1: use a generic window with the draw lambda function
    virtual void draw(std::function<void()> drawFunction, std::function<void()> menuFunction = nullptr);
    
    // Option 2: setup the draw lambda function and call the draw function without arguments
    // This is a virtual function to allow Window subclasses to protect this function
    virtual void setDrawFunction(std::function<void()> drawFunction)
    {
        if (drawFunction) {
            _drawFunction = drawFunction;
        }
    }
    
    inline void setMenuFunction(std::function<void()> menuFunction)
    {
        if (menuFunction)
        {
            _menuFunction = menuFunction;
        }
    }
    
    virtual void draw();

	inline void close() { _open = false; }
	inline void open() { _open = true; }
    inline bool isOpen() const { return _open; }

    inline void setPosition(int x, int y) { _posX = x; _posY = y; }
    inline int positionX() const { return _posX; }
    inline int positionY() const { return _posY; }
    inline void setSize(int width, int height) { _width = width; _height = height; }
    inline int width() const { return _width; }
    inline int height() const { return _height; }
    
protected:
    std::string _title = "Window";
    bool _open = true;
    std::function<void()> _drawFunction = []() {};
    std::function<void()> _menuFunction = []() {};

	int32_t _windowFlags = 0;
 
    int _posX = -1;
    int _posY = -1;
    int _width = -1;
    int _height = -1;

	void updateFlags();
};

}
}

