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

#include <bg2e/common.hpp>
#include <bg2e/ui/Window.hpp>
#include <bg2e/ui/Menu.hpp>
#include <bg2e/ui/ToolbarConfiguration.hpp>
#include <filesystem>
#include <initializer_list>
#include <optional>
#include <span>

#include <string>
#include <functional>
#include <unordered_map>
#include <vector>
#include <utility>

namespace bg2e {
namespace ui {

struct ToolbarButton {
    int32_t id = -1;
    std::string label;
    std::function<void()> action = nullptr;
    bool disabled = false;
    std::filesystem::path iconPath;
    std::string tooltip;
};

struct ToolbarLayout {
    static constexpr char Separator[] = "@separator";

    std::vector<std::string> left;
    std::vector<std::string> right;
};

class BG2E_API Toolbar : public Window {
  public:
    enum Alignment
    {
        AlignLeft,
        AlignRight
    };

    /**
     * Adds a button to the toolbar.
     * - If button.id < 0, a unique ID will be generated for this Toolbar.
     * - The button will be inserted on the side specified by 'align'.
     * - An explicit duplicate ID throws std::invalid_argument (both sides).
     *
     * @param button Button to add (reference). The id attribute in button can be changed if button.id < 0
     * @param align Button alignment (AlignLeft or AlignRight). Defaults to AlignLeft.
     * @return The button ID in this Toolbar. If a new one was generated, the generated ID is returned.
     */
    int32_t addButton(
        ToolbarButton& button,
        Alignment align = AlignLeft);

    /**
     * Move-enabled overload of 'addButton'.
     * - If button.id < 0, a unique ID will be generated for this Toolbar.
     * - The button will be inserted on the side specified by 'align'.
     * - An explicit duplicate ID throws std::invalid_argument (both sides).
     *
     * @param button Button to add (will be moved into the Toolbar).
     * @param align Button alignment (AlignLeft or AlignRight). Defaults to AlignLeft.
     * @return The button ID in this Toolbar. If a new one was generated, the generated ID is returned.
     */
    int32_t addButton(
        ToolbarButton&& button,
        Alignment align = AlignLeft);

    // Register actions in the catalog and add those with a path to the menu.
    // The optional separator follows the last action in its menu, if any.
    // Invalid IDs, duplicates and empty path components throw.
    void registerAction(ActionDefinition action, bool addSeparator = false);
    void registerAction(std::shared_ptr<const ActionDefinition> action, bool addSeparator = false);
    void registerActions(std::span<const ActionDefinition> actions, bool addSeparator = false);
    void registerActions(std::initializer_list<ActionDefinition> actions, bool addSeparator = false);

    enum class MenuSeparator
    {
        None,
        AfterAction,
        AfterSubmenu
    };

    // AfterSubmenu places the separator in the submenu's parent menu, when present.
    void registerActions(std::span<const ActionDefinition> actions, MenuSeparator separator);
    void registerActions(std::initializer_list<ActionDefinition> actions, MenuSeparator separator);

    // Register actions before setting the layout. Visible action IDs must be
    // unique and registered. ToolbarLayout::Separator may appear repeatedly.
    void setDefaultLayout(ToolbarLayout layout);

    // Enable the context menu and use local appearance and layout settings.
    // Registered actions and separators participate in layout editing.
    void enableCustomization();
    // Supply a distinct file per toolbar. Register actions and set the
    // default layout before enabling persistent customization.
    void enableCustomization(std::filesystem::path preferencesPath);

    void draw() override;

    // Configuration is resolved each draw. Either setter creates a complete
    // per-toolbar override; useSessionDefaults restores session inheritance.
    ToolbarConfiguration configuration() const;
    void setConfiguration(const ToolbarConfiguration& configuration);
    void setPresentation(ToolbarPresentation presentation);
    void setIconSize(float size);
    bool inheritsSessionDefaults() const;
    void useSessionDefaults();

    void updateButtonLabel(int32_t buttonId, const std::string& newLabel);
    void updateButtonLabel(int32_t buttonId, std::string&& newLabel);
    void updateButtonIconPath(int32_t buttonId, const std::filesystem::path& path);
    void updateButtonTooltip(int32_t buttonId, const std::string& tooltip);
    void enableButton(int32_t buttonId);
    void disableButton(int32_t buttonId);

    // Returned pointers can be invalidated by addButton. Do not mutate IDs;
    // draw rejects negative/duplicate IDs introduced through mutable access.
    ToolbarButton* findButton(int32_t buttonId);
    const ToolbarButton* findButton(int32_t buttonId) const;

  protected:
    std::optional<ToolbarConfiguration> _configuration;
    std::vector<ToolbarButton> _leftButtons;
    std::vector<ToolbarButton> _rightButtons;
    void setDrawFunction(std::function<void()> drawFunction) override;
    void draw(std::function<void()> drawFunc, std::function<void()> menuFunction = nullptr) override;

    int32_t genId();

  private:
    friend class WorkspaceCollection;
    using Window::setMenuFunction;

    void registerActionBatch(std::span<const ActionDefinition> actions, MenuSeparator separator);
    void registerActionBatch(std::span<const std::shared_ptr<const ActionDefinition>> actions,
        MenuSeparator separator, bool workspaceProjection = false);
    static std::vector<std::string> validateAction(const ActionDefinition& action);
    void registerPendingShortcuts();
    const ToolbarLayout& activeLayout() const;
    void drawContextMenu();
    void drawSettings();
    void loadCustomization();
    void saveCustomization(bool replaceExisting = true) const;

    std::unordered_map<std::string, std::shared_ptr<const ActionDefinition>> _actions;
    Menu _menu;
    WorkspaceMask _workspaceSlot = 0;
    std::vector<std::shared_ptr<const ActionDefinition>> _pendingShortcuts;
    ToolbarLayout _defaultLayout;
    std::optional<ToolbarLayout> _customLayout;
    std::filesystem::path _customizationPath;
    Window _settingsWindow;
    bool _customizationEnabled = false;
};

}
}
