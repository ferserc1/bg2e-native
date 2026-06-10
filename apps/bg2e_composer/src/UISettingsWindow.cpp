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
#include "UISettingsWindow.hpp"
#include <bg2e/ui/all.hpp>
#include <algorithm>

void UISettingsWindow::init(AppDelegate * delegate)
{
    _appDelegate = delegate;
    setTitle("UI Settings");
    setSize(300, 200);
    close();

    setDrawFunction([this]() {
        drawUI();
    });
}

void UISettingsWindow::drawUI()
{
    float scale = bg2e::ui::UserInterface::getScale();
    
    if (bg2e::ui::Input::sliderFloat("Interface Scale", &scale, 1.0f, 2.0f))
    {
        bg2e::ui::UserInterface::setScale(scale);
    }
}
