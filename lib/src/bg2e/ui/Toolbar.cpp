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
#include "detail/IconWidgets.hpp"
#include "detail/ToolbarConfiguration.hpp"
#include <imgui.h>

#include <algorithm>
#include <limits>
#include <stdexcept>
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
    int32_t id;
    std::string label;
    std::shared_ptr<Icon> icon;
    std::function<void()> action;
    IconDrawOptions options;
    detail::IconButtonMetrics metrics;
};
}

void Toolbar::draw()
{
    draw([this]()
    {
        ToolbarIdentity identity(this);
        const auto settings = configuration();
        std::unordered_set<int32_t> ids;
        const auto prepare = [&](const std::vector<ToolbarButton>& buttons)
        {
            std::vector<ToolbarEntry> entries;
            entries.reserve(buttons.size());
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
                entry.id = button.id;
                entry.label = iconOnly ? std::string{} : text;
                entry.icon = withIcon ? detail::resolveIcon(button.iconPath) : nullptr;
                entry.action = button.action;
                entry.options.disabled = button.disabled;
                entry.options.tooltip = !button.tooltip.empty() ? button.tooltip : (iconOnly ? text : std::string{});
                entry.metrics = entry.action
                    ? detail::measureIconButton(entry.label, bool(entry.icon), settings.iconSize)
                    : detail::measureIconContent(entry.label, bool(entry.icon), settings.iconSize);
                entries.push_back(std::move(entry));
            }
            return entries;
        };
        // Snapshot before callbacks run; actions may safely append/update the
        // toolbar without invalidating the entries being rendered this frame.
        const auto left = prepare(_leftButtons);
        const auto right = prepare(_rightButtons);
        const auto origin = ImGui::GetCursorScreenPos();
        const auto available = ImGui::GetContentRegionAvail().x;
        const auto spacing = ImGui::GetStyle().ItemSpacing.x;
        float rowHeight = 0.0f;
        const auto groupWidth = [&](const std::vector<ToolbarEntry>& entries)
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
        const auto leftWidth = groupWidth(left);
        const auto rightWidth = groupWidth(right);
        std::vector<std::function<void()>> actions;
        const auto render = [&](const std::vector<ToolbarEntry>& entries, float x)
        {
            for (const auto& entry : entries)
            {
                ImGui::SetCursorScreenPos(ImVec2(x, origin.y + (rowHeight - entry.metrics.height) * 0.5f));
                if (entry.action)
                {
                    if (detail::drawIconButton(std::to_string(entry.id), entry.label,
                        entry.icon, settings.iconSize, entry.options))
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
        render(left, origin.x);
        // Keep right items against the content edge when they fit. In narrow
        // windows prefer clipping to overlapping the left group or wrapping.
        const auto minimumRight = origin.x + leftWidth + (left.empty() ? 0.0f : spacing);
        render(right, std::max(minimumRight, origin.x + available - rightWidth));
        if (!left.empty() || !right.empty())
        {
            ImGui::SetCursorScreenPos(origin);
            ImGui::Dummy(ImVec2(std::max(0.0f, available), rowHeight));
        }
        for (const auto& action : actions)
        {
            action();
        }
    });
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
    std::function<void()> menuFunction)
{
    Window::draw(drawFunc, menuFunction);
    if (!_menuInitialized)
    {
        setMenuFunction([&]()
                        {
                            _menu.draw();
                        });
        _menuInitialized = false;
    }
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
