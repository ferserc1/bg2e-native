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
#include <bg2e/ui/UserInterface.hpp>

#include <algorithm>
#include <bit>
#include <stdexcept>
#include <unordered_set>

namespace bg2e::ui {

void Workspace::setup(
    uint32_t width, uint32_t height,
    Window* toolBar,
    Window* leftPanel,
    Window* rightPanel,
    Window* bottomPanel,
    Window* statusBar)
{
    _uiScale = UserInterface::getScale();

    _viewportWidth = width;
    _viewportHeight = height;
    _toolBar = toolBar;
    _leftPanel = leftPanel;
    _rightPanel = rightPanel;
    _bottomPanel = bottomPanel;
    _statusBar = statusBar;

    updateWindows();
}

void Workspace::resize(uint32_t width, uint32_t height)
{
    _viewportWidth = width;
    _viewportHeight = height;
    updateWindows();
}

void Workspace::setToolBarHeight(uint32_t logicalHeight)
{
    _toolBarHeight = logicalHeight;
    updateWindows();
}

void Workspace::draw()
{
    if (_uiScale != UserInterface::getScale())
    {
        updateWindows();
        _uiScale = UserInterface::getScale();
    }

    if (_toolBar && _drawToolBar)
    {
        _toolBar->draw();
    }

    if (_leftPanel && _drawLeftPanel)
    {
        _leftPanel->draw();
    }

    if (_rightPanel && _drawRightPanel)
    {
        _rightPanel->draw();
    }

    if (_bottomPanel && _drawBottomPanel)
    {
        _bottomPanel->draw();
    }

    if (_statusBar && _drawStatusBar)
    {
        _statusBar->draw();
    }
}

void Workspace::updateWindows()
{
    uint32_t topPadding = 0;
    uint32_t bottomPadding = 0;
    uint32_t statusPadding = 0;

    //
    // TOOL BAR
    //
    if (_toolBar && _drawToolBar)
    {
        topPadding = _toolBarHeight * UserInterface::getScale();

        _toolBar->setPosition(0, 0);
        _toolBar->setSize(_viewportWidth, _toolBarHeight * UserInterface::getScale());
        _toolBar->options = Window::Options{
            .noTitleBar = true,
            .noScrollbar = true,
            .noMenu = false,
            .noMove = true,
            .noResize = true,
            .noCollapse = true,
            .noNav = true,
            .noBringToFront = true,
            .noClose = true};
    }

    //
    // STATUS BAR
    //
    if (_statusBar && _drawStatusBar)
    {
        statusPadding = _statusBarHeight * UserInterface::getScale();

        _statusBar->setPosition(0, _viewportHeight - statusPadding);
        _statusBar->setSize(_viewportWidth, _statusBarHeight * UserInterface::getScale());

        _statusBar->options = Window::Options{
            .noTitleBar = true,
            .noScrollbar = true,
            .noMenu = true,
            .noMove = true,
            .noResize = true,
            .noCollapse = true,
            .noNav = true,
            .noBringToFront = true,
            .noClose = true};
    }

    //
    // BOTTOM PANEL
    //
    if (_bottomPanel && _drawBottomPanel)
    {
        bottomPadding = getPanelSize(_bottomPanelSize, _viewportHeight) * UserInterface::getScale();

        _bottomPanel->setPosition(
            0,
            _viewportHeight - statusPadding - bottomPadding);
        _bottomPanel->setSize(_viewportWidth, bottomPadding);
        _bottomPanel->options = Window::Options{
            .noTitleBar = false,
            .noScrollbar = false,
            .noMenu = true,
            .noMove = true,
            .noResize = true,
            .noCollapse = true,
            .noNav = true,
            .noBringToFront = true,
            .noClose = true};
    }

    //
    // LEFT PANEL
    //
    if (_leftPanel && _drawLeftPanel)
    {
        uint32_t panelWidth = getPanelSize(_leftPanelSize, _viewportWidth) * UserInterface::getScale();
        uint32_t availableHeight = _viewportHeight - topPadding - bottomPadding - statusPadding;

        _leftPanel->setPosition(0, topPadding);
        _leftPanel->setSize(panelWidth, availableHeight);
        _leftPanel->options = Window::Options{
            .noTitleBar = false,
            .noScrollbar = false,
            .noMenu = true,
            .noMove = true,
            .noResize = true,
            .noCollapse = true,
            .noNav = true,
            .noBringToFront = true,
            .noClose = true};
    }

    //
    // RIGHT PANEL
    //
    if (_rightPanel && _drawRightPanel)
    {
        uint32_t panelWidth = getPanelSize(_rightPanelSize, _viewportWidth) * UserInterface::getScale();
        uint32_t availableHeight = _viewportHeight - topPadding - bottomPadding - statusPadding;

        _rightPanel->setPosition(_viewportWidth - panelWidth, topPadding);
        _rightPanel->setSize(panelWidth, availableHeight);
        _rightPanel->options = Window::Options{
            .noTitleBar = false,
            .noScrollbar = false,
            .noMenu = true,
            .noMove = true,
            .noResize = true,
            .noCollapse = true,
            .noNav = true,
            .noBringToFront = true,
            .noClose = true};
    }
}

uint32_t Workspace::getPanelSize(PanelSize& size, uint32_t vpSize)
{
    auto result = static_cast<uint32_t>(static_cast<float>(vpSize) * size.relative);
    if (result > size.max)
    {
        return size.max;
    }
    if (result < size.min)
    {
        return size.min;
    }
    return result;
}

WorkspaceCollection::WorkspaceCollection(const WorkspaceCollection& other)
    : _workspaces(other._workspaces),
      _actions(other._actions),
      _actionBatches(other._actionBatches),
      _activeSlot(other._activeSlot)
{
}

WorkspaceCollection& WorkspaceCollection::operator=(const WorkspaceCollection& other)
{
    if (this != &other)
    {
        *this = WorkspaceCollection(other);
    }
    return *this;
}

bool WorkspaceCollection::registerWorkspace(WorkspaceMask slot, Workspace& workspace)
{
    if (!std::has_single_bit(slot))
    {
        return false;
    }

    return _workspaces.emplace(slot, &workspace).second;
}

bool WorkspaceCollection::activate(WorkspaceMask slot)
{
    if (!_workspaces.contains(slot))
    {
        return false;
    }

    if (_activeSlot != slot)
    {
        _activeSlot = slot;
        if (_shortcutSlot)
        {
            *_shortcutSlot = slot;
        }
    }
    return true;
}

WorkspaceMask WorkspaceCollection::activeSlot() const
{
    return _activeSlot;
}

Workspace* WorkspaceCollection::activeWorkspace() const
{
    const auto workspace = _workspaces.find(_activeSlot);
    return workspace != _workspaces.end() ? workspace->second : nullptr;
}

void WorkspaceCollection::registerActions(std::span<const ActionDefinition> actions,
    Toolbar::MenuSeparator separator)
{
    auto catalog = _actions;
    ActionBatch batch{{}, separator};
    batch.actions.reserve(actions.size());
    for (const auto& action : actions)
    {
        Toolbar::validateAction(action);
        if (action.workspaceMask == 0)
        {
            throw std::invalid_argument("Workspace actions require at least one workspace bit");
        }

        if (action.shortcut.key != app::KeyEvent::KeyUnknown)
        {
            const auto chord = app::Shortcuts::effectiveChord(action.shortcut);
            for (const auto& [id, existing] : catalog)
            {
                if ((action.workspaceMask & existing->workspaceMask) != 0 &&
                    chord == app::Shortcuts::effectiveChord(existing->shortcut))
                {
                    throw std::invalid_argument("Workspace actions share a shortcut in overlapping workspaces");
                }
            }
        }

        auto definition = std::make_shared<const ActionDefinition>(action);
        if (!catalog.emplace(action.id, definition).second)
        {
            throw std::invalid_argument("Workspace action ID is already in use");
        }
        batch.actions.push_back(std::move(definition));
    }
    _actionBatches.push_back(std::move(batch));
    _actions.swap(catalog);
}

void WorkspaceCollection::registerActions(std::initializer_list<ActionDefinition> actions,
    Toolbar::MenuSeparator separator)
{
    registerActions(std::span{actions.begin(), actions.size()}, separator);
}

void WorkspaceCollection::bindShortcuts(app::Shortcuts& shortcuts)
{
    if (const auto owner = shortcuts._managedOwner.lock(); owner && owner != _shortcutSlot)
    {
        throw std::invalid_argument("Shortcuts already has a live workspace collection");
    }
    auto activeSlot = _shortcutSlot ? _shortcutSlot : std::make_shared<WorkspaceMask>(_activeSlot);

    struct ChordActions {
        app::Shortcuts::Chord chord;
        std::vector<std::weak_ptr<const ActionDefinition>> actions;
    };
    std::vector<ChordActions> groups;
    for (const auto& [id, action] : _actions)
    {
        if (action->shortcut.key == app::KeyEvent::KeyUnknown)
        {
            continue;
        }
        const auto chord = app::Shortcuts::effectiveChord(action->shortcut);
        const auto group = std::ranges::find(groups, chord, &ChordActions::chord);
        if (group == groups.end())
        {
            groups.push_back({chord, {action}});
        }
        else
        {
            group->actions.emplace_back(action);
        }
    }

    std::vector<app::Shortcuts::ManagedBinding> bindings;
    bindings.reserve(groups.size());
    for (auto& group : groups)
    {
        bindings.push_back({group.chord,
            [slot = std::weak_ptr<const WorkspaceMask>(activeSlot), actions = std::move(group.actions)]
            {
                auto active = slot.lock();
                if (!active)
                {
                    return;
                }
                const auto snapshot = *active;
                active.reset();
                for (const auto& candidate : actions)
                {
                    const auto action = candidate.lock();
                    if (action && (action->workspaceMask & snapshot) != 0)
                    {
                        if (action->shortcut.allowInTextInput || !UserInterface::textInputActive())
                        {
                            action->invoke();
                        }
                        return;
                    }
                }
            }});
    }

    _shortcutSlot = std::move(activeSlot);
    shortcuts._managedOwner = _shortcutSlot;
    shortcuts._managedBindings = std::move(bindings);
}

void WorkspaceCollection::configureToolbar(WorkspaceMask slot, Toolbar& toolbar, ToolbarLayout layout) const
{
    if (!_workspaces.contains(slot))
    {
        throw std::invalid_argument("Toolbar workspace slot must be registered");
    }
    if (toolbar._workspaceSlot != 0 || !toolbar._actions.empty() || toolbar._customizationEnabled)
    {
        throw std::invalid_argument("Workspace projection requires a fresh toolbar action catalog");
    }

    std::unordered_set<std::string> usedIds;
    const auto filterLayout = [&](const std::vector<std::string>& ids)
    {
        std::vector<std::string> filtered;
        bool pendingSeparator = false;
        for (const auto& id : ids)
        {
            if (id == ToolbarLayout::Separator)
            {
                pendingSeparator = !filtered.empty();
                continue;
            }

            const auto action = _actions.find(id);
            if (action == _actions.end() || !usedIds.insert(id).second)
            {
                throw std::invalid_argument("Toolbar layout requires registered and unique action IDs");
            }
            if ((action->second->workspaceMask & slot) == 0)
            {
                continue;
            }

            if (pendingSeparator)
            {
                filtered.emplace_back(ToolbarLayout::Separator);
                pendingSeparator = false;
            }
            filtered.push_back(id);
        }
        return filtered;
    };
    ToolbarLayout filteredLayout{filterLayout(layout.left), filterLayout(layout.right)};

    for (const auto& batch : _actionBatches)
    {
        std::vector<std::shared_ptr<const ActionDefinition>> eligible;
        for (const auto& action : batch.actions)
        {
            if ((action->workspaceMask & slot) != 0)
            {
                eligible.push_back(action);
            }
        }
        toolbar.registerActionBatch(eligible, batch.separator, true);
    }
    Menu::trimSeparators(toolbar._menu._menuItems);
    toolbar.setDefaultLayout(std::move(filteredLayout));
    toolbar._workspaceSlot = slot;
}

void WorkspaceCollection::configureToolbar(WorkspaceMask slot, Toolbar& toolbar, ToolbarLayout layout,
    std::filesystem::path preferencesPath)
{
    if (preferencesPath.filename().empty())
    {
        throw std::invalid_argument("Workspace toolbar requires a preferences file");
    }

    // Resolve relative paths and existing directory aliases before reserving
    // the file so a later working directory change cannot redirect writes.
    preferencesPath = std::filesystem::absolute(preferencesPath);
    std::error_code canonicalError;
    const auto canonicalPath = std::filesystem::weakly_canonical(preferencesPath, canonicalError);
    preferencesPath = canonicalError ? preferencesPath.lexically_normal() : canonicalPath;
    for (const auto& [otherSlot, otherPath] : _toolbarPreferences)
    {
        std::error_code error;
        if (otherSlot == slot || otherPath == preferencesPath ||
            std::filesystem::equivalent(otherPath, preferencesPath, error))
        {
            throw std::invalid_argument("Workspace toolbar preferences must be distinct");
        }
    }

    const auto reservation = _toolbarPreferences.emplace(slot, preferencesPath).first;
    try
    {
        configureToolbar(slot, toolbar, std::move(layout));
        toolbar.enableCustomization(std::move(preferencesPath));
    }
    catch (...)
    {
        _toolbarPreferences.erase(reservation);
        throw;
    }
}

}
