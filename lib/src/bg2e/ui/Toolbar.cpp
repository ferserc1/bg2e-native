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

#include <bg2e/ui/Toolbar.hpp>
#include <bg2e/ui/UserInterface.hpp>
#include <bg2e/app/MainLoop.hpp>
#include "detail/IconWidgets.hpp"
#include "detail/ToolbarConfiguration.hpp"
#include <imgui.h>

#include <algorithm>
#include <limits>
#include <span>
#include <stdexcept>
#include <string_view>
#include <unordered_set>

namespace bg2e::ui {

int32_t Toolbar::addButton(
    ToolbarButton& button,
    Alignment align)
{
    auto& vec = align == AlignLeft ? _leftButtons : _rightButtons;
    if (button.id < 0)
    {
        button.id = genId();
    }
    else if (findButton(button.id))
    {
        throw std::invalid_argument("Toolbar button ID is already in use");
    }
    vec.push_back(button);
    return button.id;
}

int32_t Toolbar::addButton(
    ToolbarButton&& button,
    Alignment align)
{
    auto& vec = align == AlignLeft ? _leftButtons : _rightButtons;
    if (button.id < 0)
    {
        button.id = genId();
    }
    else if (findButton(button.id))
    {
        throw std::invalid_argument("Toolbar button ID is already in use");
    }
    vec.push_back(std::move(button));
    return button.id;
}

void Toolbar::registerAction(ActionDefinition action, bool addSeparator)
{
    registerAction(std::make_shared<const ActionDefinition>(std::move(action)), addSeparator);
}

void Toolbar::registerAction(std::shared_ptr<const ActionDefinition> action, bool addSeparator)
{
    registerActionBatch(std::span{&action, 1},
        addSeparator ? MenuSeparator::AfterAction : MenuSeparator::None);
}

void Toolbar::registerActions(std::span<const ActionDefinition> actions, bool addSeparator)
{
    registerActionBatch(actions, addSeparator ? MenuSeparator::AfterAction : MenuSeparator::None);
}

void Toolbar::registerActions(std::initializer_list<ActionDefinition> actions, bool addSeparator)
{
    registerActions(std::span{actions.begin(), actions.size()}, addSeparator);
}

void Toolbar::registerActions(std::span<const ActionDefinition> actions, MenuSeparator separator)
{
    registerActionBatch(actions, separator);
}

void Toolbar::registerActions(std::initializer_list<ActionDefinition> actions, MenuSeparator separator)
{
    registerActions(std::span{actions.begin(), actions.size()}, separator);
}

void Toolbar::registerActionBatch(std::span<const ActionDefinition> actions, MenuSeparator separator)
{
    std::vector<std::shared_ptr<const ActionDefinition>> definitions;
    definitions.reserve(actions.size());
    for (const auto& action : actions)
    {
        definitions.push_back(std::make_shared<const ActionDefinition>(action));
    }
    registerActionBatch(definitions, separator);
}

void Toolbar::registerActionBatch(std::span<const std::shared_ptr<const ActionDefinition>> actions,
    MenuSeparator separatorPlacement)
{
    std::unordered_set<std::string_view> ids;
    std::vector<std::vector<std::string>> paths;
    paths.reserve(actions.size());
    for (const auto& action : actions)
    {
        if (!action || action->id.empty() || !action->execute)
        {
            throw std::invalid_argument("Toolbar actions require an ID and an execution callback");
        }
        if (action->id == ToolbarLayout::Separator)
        {
            throw std::invalid_argument("Toolbar action ID is reserved for separators");
        }
        if (_actions.contains(action->id) || !ids.insert(action->id).second)
        {
            throw std::invalid_argument("Toolbar action ID is already in use");
        }

        auto& path = paths.emplace_back();
        std::string_view remaining = action->path;
        if (remaining.empty())
        {
            continue;
        }
        for (;;)
        {
            const auto separator = remaining.find('|');
            const auto parent = remaining.substr(0, separator);
            if (parent.find_first_not_of(" \t\r\n") == std::string_view::npos)
            {
                throw std::invalid_argument("Toolbar menu paths require nonempty parent names");
            }
            path.emplace_back(parent);
            if (separator == std::string_view::npos)
            {
                break;
            }
            remaining.remove_prefix(separator + 1);
        }
    }

    for (std::size_t i = 0; i < actions.size(); ++i)
    {
        const auto& action = actions[i];
        _actions.emplace(action->id, action);
        if (!paths[i].empty())
        {
            const bool addSeparator = i + 1 == actions.size() && separatorPlacement != MenuSeparator::None;
            _menu.addAction(paths[i], action, addSeparator, separatorPlacement == MenuSeparator::AfterSubmenu);
        }
        if (action->shortcut.key != app::KeyEvent::KeyUnknown)
        {
            _pendingShortcuts.push_back(action);
        }
    }
    registerPendingShortcuts();
}

void Toolbar::registerPendingShortcuts()
{
    if (_pendingShortcuts.empty() || !app::MainLoop::current())
    {
        return;
    }
    for (const auto& action : _pendingShortcuts)
    {
        ActionDefinition::registerShortcut(app::MainLoop::shortcuts(), action);
    }
    _pendingShortcuts.clear();
}

void Toolbar::setDefaultLayout(ToolbarLayout layout)
{
    std::unordered_set<std::string> used;
    const auto validate = [&](const std::vector<std::string>& side)
    {
        for (const auto& id : side)
        {
            if (id != ToolbarLayout::Separator && (!_actions.contains(id) || !used.insert(id).second))
            {
                throw std::invalid_argument("Toolbar layout requires registered and unique action IDs");
            }
        }
    };
    validate(layout.left);
    validate(layout.right);
    _defaultLayout = std::move(layout);
}

namespace {
class ToolbarIdentity {
  public:
    explicit ToolbarIdentity(const Toolbar* toolbar)
    {
        ImGui::PushID(toolbar);
    }
    ~ToolbarIdentity()
    {
        ImGui::PopID();
    }
};

struct ToolbarEntry {
    std::string id;
    std::string label;
    std::string overflowLabel;
    std::shared_ptr<Icon> icon;
    std::function<void()> action;
    IconDrawOptions options;
    detail::IconButtonMetrics metrics;
    bool selected = false;
    bool separator = false;
};
}

void Toolbar::draw()
{
    registerPendingShortcuts();
    draw([this]()
    {
        ToolbarIdentity identity(this);
        const auto settings = configuration();
        std::unordered_set<int32_t> ids;
        const auto prepare = [&](const std::vector<ToolbarButton>& buttons, const std::vector<std::string>& actionIds)
        {
            std::vector<ToolbarEntry> entries;
            entries.reserve(buttons.size() + actionIds.size());
            for (const auto& button : buttons)
            {
                if (button.id < 0 || !ids.insert(button.id).second)
                {
                    throw std::invalid_argument("Toolbar button IDs must be nonnegative and unique");
                }
                const bool withIcon = settings.presentation != ToolbarPresentation::Text && !button.iconPath.empty();
                const bool iconOnly = withIcon && settings.presentation == ToolbarPresentation::IconsWithTooltips;
                // Preserve the old button label's hidden suffix convention;
                // actionless Text entries have always treated labels literally.
                const auto text = button.action ? button.label.substr(0, button.label.find("##")) : button.label;
                ToolbarEntry entry;
                entry.id = std::to_string(button.id);
                entry.label = iconOnly ? std::string{} : text;
                entry.overflowLabel = text;
                entry.icon = withIcon ? detail::resolveIcon(button.iconPath) : nullptr;
                entry.action = button.action;
                entry.options.disabled = button.disabled;
                entry.options.tooltip = !button.tooltip.empty() ? button.tooltip : (iconOnly ? text : std::string{});
                entry.metrics = entry.action
                    ? detail::measureIconButton(entry.label, bool(entry.icon), settings.iconSize)
                    : detail::measureIconContent(entry.label, bool(entry.icon), settings.iconSize);
                entries.push_back(std::move(entry));
            }
            for (const auto& id : actionIds)
            {
                if (id == ToolbarLayout::Separator)
                {
                    ToolbarEntry entry{};
                    entry.metrics.width = std::max(1.0f, UserInterface::getScale());
                    entry.metrics.height = ImGui::GetFrameHeight();
                    entry.separator = true;
                    entries.push_back(std::move(entry));
                    continue;
                }
                const auto found = _actions.find(id);
                if (found == _actions.end())
                {
                    continue;
                }
                const auto action = found->second;
                auto text = action->displayLabel();
                text = text.substr(0, text.find("##"));
                ToolbarEntry entry;
                entry.id = "action:" + id;
                if (settings.presentation != ToolbarPresentation::Text && !action->iconPath.empty())
                {
                    entry.icon = detail::resolveIcon(action->iconPath);
                }
                const bool iconOnly = entry.icon && settings.presentation == ToolbarPresentation::IconsWithTooltips;
                entry.label = iconOnly ? std::string{} : text;
                entry.overflowLabel = text;
                entry.action = [action] { action->invoke(); };
                entry.options.disabled = !action->isEnabled();
                entry.options.tooltip = !action->tooltip.empty() ? action->tooltip : (iconOnly ? text : std::string{});
                entry.metrics = detail::measureIconButton(entry.label, bool(entry.icon), settings.iconSize);
                entry.selected = action->isChecked();
                entries.push_back(std::move(entry));
            }
            return entries;
        };
        // Snapshot before callbacks run; actions may safely append/update the
        // toolbar without invalidating the entries being rendered this frame.
        const auto& layout = activeLayout();
        const auto left = prepare(_leftButtons, layout.left);
        const auto right = prepare(_rightButtons, layout.right);
        const auto origin = ImGui::GetCursorScreenPos();
        const auto available = ImGui::GetContentRegionAvail().x;
        const auto spacing = ImGui::GetStyle().ItemSpacing.x;
        float rowHeight = 0.0f;
        const auto groupWidth = [&](std::span<const ToolbarEntry> entries)
        {
            float width = 0.0f;
            for (const auto& entry : entries)
            {
                width += entry.metrics.width;
                rowHeight = std::max(rowHeight, entry.metrics.height);
            }
            if (!entries.empty())
            {
                width += spacing * float(entries.size() - 1);
            }
            return width;
        };
        auto leftWidth = groupWidth(left);
        auto rightWidth = groupWidth(right);
        auto visibleLeft = std::span<const ToolbarEntry>(left);
        auto visibleRight = std::span<const ToolbarEntry>(right);
        auto hiddenLeft = std::span<const ToolbarEntry>{};
        auto hiddenRight = std::span<const ToolbarEntry>{};
        const float groupSpacing = left.empty() || right.empty() ? 0.0f : spacing;
        bool hasOverflow = _customizationEnabled && leftWidth + rightWidth + groupSpacing > available;
        auto rightEdge = origin.x + available;
        constexpr auto overflowLabel = "More Tools";
        if (hasOverflow)
        {
            const auto overflow = detail::measureIconButton(overflowLabel, false, settings.iconSize);
            rowHeight = std::max(rowHeight, overflow.height);
            rightEdge -= overflow.width + spacing;

            const auto fit = [&](std::span<const ToolbarEntry> entries, float budget, float& width)
            {
                width = 0.0f;
                size_t count = 0;
                for (const auto& entry : entries)
                {
                    const auto next = width + (count == 0 ? 0.0f : spacing) + entry.metrics.width;
                    if (next > budget)
                    {
                        break;
                    }
                    width = next;
                    ++count;
                }
                return entries.first(count);
            };
            const auto budget = rightEdge - origin.x;
            visibleRight = fit(visibleRight, budget, rightWidth);
            visibleLeft = fit(visibleLeft, budget - rightWidth - (visibleRight.empty() ? 0.0f : spacing), leftWidth);
            hiddenLeft = std::span<const ToolbarEntry>(left).subspan(visibleLeft.size());
            hiddenRight = std::span<const ToolbarEntry>(right).subspan(visibleRight.size());
            const auto isTool = [](const ToolbarEntry& entry)
            {
                return !entry.separator;
            };
            hasOverflow = std::ranges::any_of(hiddenLeft, isTool) || std::ranges::any_of(hiddenRight, isTool);
            if (!hasOverflow)
            {
                rightEdge = origin.x + available;
            }
        }
        std::vector<std::function<void()>> actions;
        const auto render = [&](std::span<const ToolbarEntry> entries, float x)
        {
            for (const auto& entry : entries)
            {
                if (entry.separator)
                {
                    const float inset = ImGui::GetStyle().FramePadding.y;
                    ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(x, origin.y + inset),
                        ImVec2(x + entry.metrics.width, origin.y + rowHeight - inset),
                        ImGui::GetColorU32(ImGuiCol_Separator));
                    x += entry.metrics.width + spacing;
                    continue;
                }
                ImGui::SetCursorScreenPos(ImVec2(x, origin.y + (rowHeight - entry.metrics.height) * 0.5f));
                if (entry.action)
                {
                    if (detail::drawIconButton(entry.id, entry.label,
                        entry.icon, settings.iconSize, entry.options, entry.selected))
                    {
                        actions.push_back(entry.action);
                    }
                }
                else
                {
                    detail::drawIconContent(entry.label, entry.icon, settings.iconSize, entry.options);
                }
                x += entry.metrics.width + spacing;
            }
        };
        render(visibleLeft, origin.x);
        const auto minimumRight = origin.x + leftWidth + (visibleLeft.empty() ? 0.0f : spacing);
        render(visibleRight, std::max(minimumRight, rightEdge - rightWidth));
        if (hasOverflow)
        {
            ImGui::SetCursorScreenPos(ImVec2(std::max(origin.x, rightEdge + spacing),
                origin.y + (rowHeight - ImGui::GetFrameHeight()) * 0.5f));
            if (ImGui::Button(overflowLabel))
            {
                ImGui::OpenPopup("ToolbarOverflow");
            }
            if (ImGui::BeginPopup("ToolbarOverflow"))
            {
                const auto drawHidden = [&](std::span<const ToolbarEntry> entries)
                {
                    bool drewTool = false;
                    bool pendingSeparator = false;
                    for (const auto& entry : entries)
                    {
                        if (entry.separator)
                        {
                            pendingSeparator = drewTool;
                            continue;
                        }
                        if (pendingSeparator)
                        {
                            Menu::separator();
                            pendingSeparator = false;
                        }
                        if (Menu::menuItem(entry.overflowLabel + "###" + entry.id, {},
                            entry.selected, entry.action && !entry.options.disabled))
                        {
                            actions.push_back(entry.action);
                        }
                        drewTool = true;
                    }
                };
                drawHidden(hiddenLeft);
                drawHidden(hiddenRight);
                ImGui::EndPopup();
            }
        }
        if (!left.empty() || !right.empty())
        {
            ImGui::SetCursorScreenPos(origin);
            ImGui::Dummy(ImVec2(std::max(0.0f, available), rowHeight));
        }
        for (const auto& action : actions)
        {
            action();
        }
        if (_customizationEnabled)
        {
            drawContextMenu();
        }
    });

    if (_customizationEnabled && isOpen() && _settingsWindow.isOpen())
    {
        _settingsWindow.draw([this] { drawSettings(); });
    }
}

ToolbarConfiguration Toolbar::configuration() const
{
    return _configuration ? *_configuration : UserInterface::getToolbarConfiguration();
}

void Toolbar::setConfiguration(const ToolbarConfiguration& configuration)
{
    detail::validateToolbarConfiguration(configuration);
    _configuration = configuration;
}

void Toolbar::setPresentation(ToolbarPresentation presentation)
{
    auto settings = configuration();
    settings.presentation = presentation;
    setConfiguration(settings);
}

void Toolbar::setIconSize(float size)
{
    auto settings = configuration();
    settings.iconSize = size;
    setConfiguration(settings);
}

bool Toolbar::inheritsSessionDefaults() const
{
    return !_configuration.has_value();
}

void Toolbar::useSessionDefaults()
{
    _configuration.reset();
}

void Toolbar::updateButtonLabel(int32_t buttonId, const std::string& newLabel)
{
    auto btn = findButton(buttonId);
    if (btn)
    {
        btn->label = newLabel;
    }
}

void Toolbar::updateButtonLabel(int32_t buttonId, std::string&& newLabel)
{
    auto btn = findButton(buttonId);
    if (btn)
    {
        btn->label = std::move(newLabel);
    }
}

void Toolbar::updateButtonIconPath(int32_t buttonId, const std::filesystem::path& path)
{
    if (auto* button = findButton(buttonId))
    {
        button->iconPath = path;
    }
}

void Toolbar::updateButtonTooltip(int32_t buttonId, const std::string& tooltip)
{
    if (auto* button = findButton(buttonId))
    {
        button->tooltip = tooltip;
    }
}

void Toolbar::enableButton(int32_t buttonId)
{
    auto btn = findButton(buttonId);
    if (btn)
    {
        btn->disabled = false;
    }
}

void Toolbar::disableButton(int32_t buttonId)
{
    auto btn = findButton(buttonId);
    if (btn)
    {
        btn->disabled = true;
    }
}

ToolbarButton* Toolbar::findButton(int32_t buttonId)
{
    return const_cast<ToolbarButton*>(std::as_const(*this).findButton(buttonId));
}

const ToolbarButton* Toolbar::findButton(int32_t buttonId) const
{
    for (const auto& button : _leftButtons)
    {
        if (button.id == buttonId)
        {
            return &button;
        }
    }
    for (const auto& button : _rightButtons)
    {
        if (button.id == buttonId)
        {
            return &button;
        }
    }
    return nullptr;
}

void Toolbar::setDrawFunction(std::function<void()> /* drawFunction */)
{
    // Not used
}

void Toolbar::draw(
    std::function<void()> drawFunc,
    std::function<void()> /* menuFunction */)
{
    _menuFunction = nullptr;
    Window::draw(std::move(drawFunc), [this]()
    {
        _menu.draw();
    });
}

int32_t Toolbar::genId()
{
    // Generate a unique button id not used by any button in _leftButtons or _rightButtons
    auto idInUse = [this](int32_t candidate) -> bool
    {
        for (const auto& b : _leftButtons)
        {
            if (b.id == candidate)
            {
                return true;
            }
        }
        for (const auto& b : _rightButtons)
        {
            if (b.id == candidate)
            {
                return true;
            }
        }
        return false;
    };

    int32_t candidate = 0;
    while (idInUse(candidate))
    {
        if (candidate == std::numeric_limits<int32_t>::max())
        {
            throw std::overflow_error("No toolbar button ID is available");
        }
        ++candidate;
    }
    return candidate;
}

}
