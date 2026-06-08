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

#include <bg2e.hpp>

#include "EnvironmentSettings.hpp"
#include "SubmeshWindow.hpp"

class  AppDelegate;
class UISettingsWindow;
class UIRenderSettingsWindow;

class ToolBar : public bg2e::ui::Toolbar {
public:
    virtual ~ToolBar() = default;

    void init(AppDelegate * delegate, UISettingsWindow * uiSettings, UIRenderSettingsWindow * renderSettings);
    
protected:

    AppDelegate * _appDelegate = nullptr;
    UISettingsWindow * _uiSettingsWindow = nullptr;
    UIRenderSettingsWindow * _renderSettingsWindow = nullptr;
};
