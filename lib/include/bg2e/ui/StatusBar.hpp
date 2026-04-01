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

#include <bg2e/ui/Window.hpp>

#include <vector>
#include <string>
#include <memory>

namespace bg2e {
namespace ui {

class BG2E_API StatusItem {
public:
    inline void setText(const std::string & text) { _text = text; }
    inline void setText(std::string&& text) { _text = std::move(text); }
    inline const std::string & getText() const { return _text; }

protected:
    std::string _text;
};

class BG2E_API StatusBar : public Window {
public:

    enum Alignment {
        AlignLeft,
        AlignRight
    };

    void draw() override;

    inline void addItem(std::shared_ptr<StatusItem> item, Alignment align = AlignLeft)
    {
        align == AlignLeft
            ? _leftItems.push_back(item)
            : _rightItems.push_back(item);
    }

protected:
    std::vector<std::shared_ptr<StatusItem>> _leftItems;
    std::vector<std::shared_ptr<StatusItem>> _rightItems;

    void setDrawFunction(std::function<void()> drawFunction) override;
    void draw(std::function<void()> drawFunc, std::function<void()> menuFunction = nullptr) override;

};

}
}