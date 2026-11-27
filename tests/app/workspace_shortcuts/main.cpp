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

#include <bg2e/ui/Workspace.hpp>

#include <stdexcept>

using bg2e::app::KeyEvent;
using bg2e::app::Shortcuts;
using namespace bg2e::ui;

namespace {

void require(bool condition, const char* message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

}

int main()
{
    Shortcuts shortcuts;
    Workspace scene;
    Workspace volume;
    int sceneDeletes = 0;
    int volumeDeletes = 0;
    int saves = 0;
    int standalone = 0;
    bool volumeDeleteEnabled = false;

    shortcuts.addShortcutMapper({
        .key = KeyEvent::KeyDelete,
        .handler = [&] { ++standalone; }
    });

    {
        WorkspaceCollection workspaces;
        require(workspaces.registerWorkspace(Workspace0, scene), "Register scene");
        require(workspaces.registerWorkspace(Workspace31, volume), "Register volume");
        require(workspaces.activate(Workspace0), "Activate scene");
        workspaces.registerActions({
            {.id = "scene.delete", .execute = [&]
                {
                    ++sceneDeletes;
                    require(workspaces.activate(Workspace31), "Switch to volume");
                },
                .shortcut = {.key = KeyEvent::KeyDelete}, .workspaceMask = Workspace0},
            {.id = "volume.delete", .execute = [&] { ++volumeDeletes; },
                .enabled = [&] { return volumeDeleteEnabled; },
                .shortcut = {.key = KeyEvent::KeyDelete}, .workspaceMask = Workspace31},
            {.id = "save", .execute = [&] { ++saves; },
                .shortcut = {.key = KeyEvent::KeyS, .cmdOrCtrlModifier = true}}
        });

        bool conflictRejected = false;
        try
        {
            workspaces.registerActions({
                {.id = "new", .execute = [] {}},
                {.id = "conflict", .execute = [] {},
#if defined(__APPLE__)
                    .shortcut = {.key = KeyEvent::KeyS, .cmdOrCtrlModifier = true}}
#else
                    .shortcut = {.ctrlModifier = true, .key = KeyEvent::KeyS}}
#endif
            });
        }
        catch (const std::invalid_argument&)
        {
            conflictRejected = true;
        }
        require(conflictRejected, "Reject overlapping effective chord");
        workspaces.registerActions({{.id = "new", .execute = [] {}}});

        workspaces.bindShortcuts(shortcuts);
        workspaces.bindShortcuts(shortcuts);
        shortcuts.keyDown(KeyEvent::KeyDelete);
        require(sceneDeletes == 0 && standalone == 0, "Dispatch on release");
        shortcuts.keyUp(KeyEvent::KeyDelete);
        require(sceneDeletes == 1 && volumeDeletes == 0, "One action after workspace switch");
        shortcuts.keyUp(KeyEvent::KeyDelete);
        require(volumeDeletes == 0, "Disabled action");
        volumeDeleteEnabled = true;
        shortcuts.keyUp(KeyEvent::KeyDelete);
        require(volumeDeletes == 1 && standalone == 3, "Volume and standalone dispatch");

#if defined(__APPLE__)
        const auto saveModifier = KeyEvent::KeyLeftSuper;
#else
        const auto saveModifier = KeyEvent::KeyLeftControl;
#endif
        shortcuts.keyDown(saveModifier);
        shortcuts.keyUp(KeyEvent::KeyS);
        require(workspaces.activate(Workspace0), "Return to scene");
        shortcuts.keyUp(KeyEvent::KeyS);
        shortcuts.keyUp(saveModifier);
        require(saves == 2, "One shared Save per release in both workspaces");
    }

    shortcuts.keyUp(KeyEvent::KeyDelete);
    require(sceneDeletes == 1 && volumeDeletes == 1 && standalone == 4, "Expired collection");

    WorkspaceCollection replacement;
    require(replacement.registerWorkspace(Workspace0, scene), "Register replacement scene");
    require(replacement.activate(Workspace0), "Activate replacement scene");
    replacement.registerActions({{.id = "delete", .execute = [&] { ++sceneDeletes; },
        .shortcut = {.key = KeyEvent::KeyDelete, .allowInTextInput = false}}});
    replacement.bindShortcuts(shortcuts);
    shortcuts.keyUp(KeyEvent::KeyDelete);
    require(sceneDeletes == 2 && standalone == 5, "One replacement action without focused text");
}
