/*
 *    business grade graphic engine (bg2 engine)
 *    Copyright (C) 2026  Fernando Serrano Carpena
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

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
    static const char * s_defaultWindowTitle;
    static uint32_t s_windowIndex;

    std::string _title = s_defaultWindowTitle;
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

