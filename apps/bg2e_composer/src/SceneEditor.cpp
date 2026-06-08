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
#include "SceneEditor.hpp"
#include "AppDelegate.hpp"
#include "StageScene.hpp"

void SceneEditor::init(AppDelegate * delegate)
{
    _appDelegate = delegate;
    setTitle("Scene");

    _nodeEditor.init(delegate->engine());
    _nodeEditor.onChanged([&]() {
        _appDelegate->stage()->document()->setUnsavedChanges(true);
        auto scene = _appDelegate->stage()->sceneRoot()->scene();
        if (scene) scene->updateAll();
    });

    setDrawFunction([&]() {
        _sceneTree.setRootNode(_appDelegate->stage()->editableRoot().get());

        const float avail = bg2e::ui::BasicWidgets::getContentRegionAvailHeight();
        const float treeHeight = avail * 0.5f;

        bg2e::ui::BasicWidgets::beginChild("scene_tree", 0.0f, treeHeight);
        _sceneTree.draw();
        bg2e::ui::BasicWidgets::endChild();

        bg2e::ui::BasicWidgets::beginChild("node_editor");
        _nodeEditor.draw();
        bg2e::ui::BasicWidgets::endChild();
    });
}

void SceneEditor::cleanup()
{
    _nodeEditor.onChanged(nullptr);
}
