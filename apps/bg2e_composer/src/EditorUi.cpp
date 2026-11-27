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

#include "EditorUi.hpp"
#include "workspaces/common/SettingsWindows.hpp"
#include "workspaces/common/WorkspaceSlots.hpp"
#include "workspaces/scene/SceneWorkspace.hpp"

#include <bg2e/app/MainLoop.hpp>

namespace bg2e_composer {

EditorUi::EditorUi(const std::shared_ptr<AppDelegate>& delegate)
    : _delegate(delegate),
      _settings(std::make_shared<workspaces::common::SettingsWindows>(delegate)),
      _scene(std::make_shared<workspaces::scene::SceneWorkspace>(delegate, _settings))
{
    _workspaces.registerWorkspace(workspaces::SceneSlot, _scene->layout());
}

void EditorUi::init(bg2e::draw::Engine* engine, bg2e::ui::UserInterface* session)
{
    _settings->init();
    _scene->init(engine, session, uiWidth(), uiHeight(), _workspaces);
    _workspaces.activate(workspaces::SceneSlot);
    _workspaces.bindShortcuts(bg2e::app::MainLoop::shortcuts());

    if (const auto delegate = _delegate.lock())
    {
        delegate->onSceneShortcutScope([weak = weak_from_this()]
        {
            const auto ui = weak.lock();
            return ui && ui->_workspaces.activeSlot() == workspaces::SceneSlot;
        });
    }
}

bool EditorUi::requestWorkspace(bg2e::ui::WorkspaceMask destination)
{
    if (destination != 0 && destination == _workspaces.activeSlot())
    {
        return true;
    }
    if (destination != workspaces::SceneSlot || !_scene->layout().isValid())
    {
        return false;
    }

    const auto delegate = _delegate.lock();
    return delegate && delegate->queueWorkspaceTransition(
        [weak = weak_from_this(), destination]() -> std::function<void()>
        {
            auto ui = weak.lock();
            if (!ui || !ui->_scene->layout().isValid() || ui->_workspaces.activeSlot() == destination)
            {
                return {};
            }
            // SceneSlot was registered at construction and cannot be removed.
            return [ui = std::move(ui), destination]
            {
                ui->_workspaces.activate(destination);
            };
        });
}

void EditorUi::drawUI()
{
    if (_workspaces.activeWorkspace() == &_scene->layout())
    {
        _scene->draw(uiWidth(), uiHeight());
    }
}

}
