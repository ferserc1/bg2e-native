#pragma once

#include <bg2e/common.hpp>

#include <functional>
#include <string>

namespace bg2e {
namespace ui {

class BG2E_API Window {
public:
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

    void draw(std::function<void()> drawFunction);

	inline void close() { _open = false; }
	inline void open() { _open = true; }

    inline void setPosition(int x, int y) { _posX = x; _posY = y; }
    inline int positionX() const { return _posX; }
    inline int positionY() const { return _posY; }
    inline void setSize(int width, int height) { _width = width; _height = height; }
    inline int width() const { return _width; }
    inline int height() const { return _height; }
    
protected:
    std::string _title = "Window";
    bool _open = true;
    

	int32_t _windowFlags = 0;
 
    int _posX = -1;
    int _posY = -1;
    int _width = -1;
    int _height = -1;

	void updateFlags();
};

}
}

