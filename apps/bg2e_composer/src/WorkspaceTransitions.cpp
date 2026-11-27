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

#include "AppDelegate.hpp"

#include <utility>

namespace bg2e_composer {

bool AppDelegate::workspaceTransitionIdle() const
{
    const auto* doc = document();
    return doc && doc->root && doc->commands && !sceneLoading() && !_pendingIo &&
        !_imports.busy() && !_importPublicationPending && !_quitPending &&
        !_settlingUiInteractions && _deferredReads.empty() && !doc->commands->pending() &&
        !doc->commands->history().faulted() &&
        doc->pendingCompatibility == bg2e::app::CommandCompatibility::Unspecified;
}

bool AppDelegate::queueWorkspaceTransition(PrepareWorkspace prepare)
{
    if (!prepare || _workspaceTransition || !workspaceTransitionIdle())
    {
        return false;
    }
    _workspaceTransition = WorkspaceTransition{document()->generation, std::move(prepare)};
    requestSceneFrame();
    return true;
}

void AppDelegate::processWorkspaceTransition()
{
    auto transition = std::exchange(_workspaceTransition, std::nullopt);
    if (!transition)
    {
        return;
    }
    if (!workspaceTransitionIdle() || document()->generation != transition->generation)
    {
        _status = "Workspace switch requires an idle document";
        return;
    }

    const auto activate = transition->prepare();
    if (!activate)
    {
        _status = "The requested workspace is not ready";
        return;
    }

    // Cancel previews so a switch does not create edits or invalidate redo.
    // Asynchronous cancellation refuses this request until settlement finishes.
    if (_settleUiInteractions && !_settleUiInteractions(false))
    {
        _status = "The active UI edit could not be settled for the workspace switch";
        return;
    }
    finishTransformInteraction(false);
    if (!workspaceTransitionIdle() || document()->generation != transition->generation)
    {
        _status = "Workspace switch requires an idle document";
        return;
    }

    const auto result = document()->commands->discardWorkspaceHistory();
    if (!result.accepted())
    {
        _status = std::string(result.message);
        return;
    }
    activate();
    _stage->synchronizeDirty();
    requestSceneFrame();
}

}
