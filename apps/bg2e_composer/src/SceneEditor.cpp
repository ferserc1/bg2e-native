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

#include <bg2e/scene/Chain.hpp>

void SceneEditor::init(AppDelegate * delegate)
{
    _appDelegate = delegate;
    setTitle("Scene");

    _nodeEditor.onChanged([&]() {
        _appDelegate->stage()->document()->setUnsavedChanges(true);
        auto scene = _appDelegate->stage()->sceneRoot()->scene();
        if (scene) scene->updateAll();
    });

    setDrawFunction([&]() {
        _sceneTree.setRootNode(_appDelegate->stage()->editableRoot().get());

        const float avail = bg2e::ui::Layout::getContentRegionAvailHeight();
        const float treeHeight = avail * 0.5f;

        bg2e::ui::Layout::beginChild("scene_tree", 0.0f, treeHeight);
        _sceneTree.draw();
        bg2e::ui::Layout::endChild();

        bg2e::ui::Layout::beginChild("node_editor");
        _nodeEditor.draw();
        bg2e::ui::Layout::endChild();
    });
}

void SceneEditor::drawChainComponentControls()
{
    auto* node = _nodeEditor.node();
    if (!node)
    {
        return;
    }

    auto* chain = node->getComponent<bg2e::scene::ChainComponent>();
    if (!chain)
    {
        if (bg2e::ui::Button::button("Add Chain Component"))
        {
            node->addComponent(new bg2e::scene::ChainComponent());
            _appDelegate->stage()->document()->setUnsavedChanges(true);
            if (auto* scene = _appDelegate->stage()->sceneRoot()->scene())
            {
                scene->updateAll();
            }
        }
    }
    else if (bg2e::ui::Button::button("Remove Chain Component"))
    {
        node->removeComponent(chain->shared_from_this());
        _appDelegate->stage()->document()->setUnsavedChanges(true);
        if (auto* scene = _appDelegate->stage()->sceneRoot()->scene())
        {
            scene->updateAll();
        }
    }
}

void SceneEditor::cleanup()
{
    _nodeEditor.onChanged(nullptr);
}
