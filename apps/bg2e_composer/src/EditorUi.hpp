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

#include <bg2e/ui/UserInterfaceDelegate.hpp>
#include <bg2e/ui/Workspace.hpp>

#include <memory>

namespace bg2e_composer {

class AppDelegate;

namespace workspaces::common { class SettingsWindows; }

namespace workspaces::scene {
class SceneWorkspace;
}

class EditorUi final : public bg2e::ui::UserInterfaceDelegate, public std::enable_shared_from_this<EditorUi> {
  public:
    explicit EditorUi(const std::shared_ptr<AppDelegate>& delegate);

    void init(bg2e::draw::Engine* engine, bg2e::ui::UserInterface* session) override;
    void drawUI() override;

    bool requestWorkspace(bg2e::ui::WorkspaceMask destination);

  private:
    std::weak_ptr<AppDelegate> _delegate;
    std::shared_ptr<workspaces::common::SettingsWindows> _settings;
    std::shared_ptr<workspaces::scene::SceneWorkspace> _scene;
    bg2e::ui::WorkspaceCollection _workspaces;
};

}
