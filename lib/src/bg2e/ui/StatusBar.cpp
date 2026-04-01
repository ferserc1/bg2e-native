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

#include <bg2e/ui/StatusBar.hpp>
#include <bg2e/ui/BasicWidgets.hpp>

namespace bg2e::ui {

void StatusBar::draw()
{
    draw([&]()
    {
        auto textSize = BasicWidgets::calcTextHeight("text") + BasicWidgets::getItemVerticalSpacing();
        BasicWidgets::padding(0, height() / 2 - textSize);

        auto sameLine = false;
        for (auto & item : _leftItems)
        {
            BasicWidgets::text(item->getText(), sameLine);
            sameLine = true;
        }

        auto rightButtonsSize = 0;
        for (auto & item : _rightItems)
        {
            rightButtonsSize += BasicWidgets::calcTextWidth(item->getText()) +
                BasicWidgets::getItemHorizontalSpacing();
        }

        auto txtIndex = 0;
        for (auto & item : _rightItems)
        {
            txtIndex == 0 ? BasicWidgets::sameLine(-rightButtonsSize) : BasicWidgets::sameLine();
            BasicWidgets::text(item->getText(), false);
            ++txtIndex;
        }
    });
}

void StatusBar::setDrawFunction([[maybe_unused]] std::function<void()> drawFunction)
{
}

void StatusBar::draw(
    std::function<void()> drawFunc,
    std::function<void()> menuFunction
) {
    Window::draw(drawFunc, menuFunction);
}

}