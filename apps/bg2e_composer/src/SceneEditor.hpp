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
#include <bg2e/ui/SceneTree.hpp>
#include <bg2e/ui/NodeEditor.hpp>

class AppDelegate;

class SceneEditor : public bg2e::ui::Window {
public:
    void init(AppDelegate * delegate);
    void cleanup();

    bg2e::ui::SceneTree & sceneTree() { return _sceneTree; }
    bg2e::ui::NodeEditor & nodeEditor() { return _nodeEditor; }

protected:
    AppDelegate * _appDelegate = nullptr;
    bg2e::ui::SceneTree _sceneTree;
    bg2e::ui::NodeEditor _nodeEditor;
};
