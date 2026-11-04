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

#include "IconWidgets.hpp"
#include "IconAccess.hpp"
#include "ImageDraw.hpp"
#include "ImageRegistry.hpp"
#include <bg2e/ui/IconManager.hpp>
#include <bg2e/ui/UserInterface.hpp>
#include <bg2e/gpu/Image.hpp>
#include <imgui.h>
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace bg2e::ui::detail {
namespace {
float scaledIconSize(float size)
{
    const auto scaled = size * UserInterface::getScale();
    if (!std::isfinite(size) || size <= 0.0f || !std::isfinite(scaled) || scaled <= 0.0f)
    {
        throw std::invalid_argument("Icon size must be finite and positive");
    }
    return scaled;
}

class DisabledScope {
  public:
    explicit DisabledScope(bool disabled)
    {
        ImGui::BeginDisabled(disabled);
    }
    ~DisabledScope()
    {
        ImGui::EndDisabled();
    }
};

class IdScope {
  public:
    explicit IdScope(const std::string& id)
    {
        ImGui::PushID(id.data(), id.data() + id.size());
    }
    ~IdScope()
    {
        ImGui::PopID();
    }
};

class ClipScope {
  public:
    ClipScope(const ImVec2& min, const ImVec2& max) : _drawList(ImGui::GetWindowDrawList())
    {
        _drawList->PushClipRect(min, max, true);
    }
    ~ClipScope()
    {
        _drawList->PopClipRect();
    }

  private:
    ImDrawList* _drawList;
};

void tooltip(const std::string& text)
{
    if (!text.empty() && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
    {
        ImGui::SetTooltip("%s", text.c_str());
    }
}

}

void paintIcon(const Icon& icon, float x, float y, float boxSize,
    const base::Color& tint, ImDrawList* drawList)
{
    if (!IconAccess::belongsToCurrentSession(icon))
    {
        return;
    }
    auto* target = drawList ? drawList : ImGui::GetWindowDrawList();
    const auto entry = IconAccess::image(icon);
    if (imageValid(entry))
    {
        const auto dimensions = entry->image->size();
        if (dimensions.width > 0 && dimensions.height > 0)
        {
            const auto ratio = boxSize / float(std::max(dimensions.width, dimensions.height));
            const auto width = float(dimensions.width) * ratio;
            const auto height = float(dimensions.height) * ratio;
            drawImageAt(entry, x + (boxSize - width) * 0.5f, y + (boxSize - height) * 0.5f,
                width, height, tint, target);
            return;
        }
    }
    // The owning session has already been checked. Only asset fallback uses
    // this path: a stale handle must never be shown as a missing source.
    if (icon.isFallback())
    {
        const auto fontSize = std::min(ImGui::GetFontSize(), boxSize);
        const auto textSize = ImGui::CalcTextSize("?");
        const auto ratio = fontSize / ImGui::GetFontSize();
        auto color = ImGui::GetStyleColorVec4(ImGuiCol_Text);
        color.x *= tint.r;
        color.y *= tint.g;
        color.z *= tint.b;
        color.w *= tint.a;
        target->AddText(ImGui::GetFont(), fontSize,
            ImVec2(x + (boxSize - textSize.x * ratio) * 0.5f,
                y + (boxSize - textSize.y * ratio) * 0.5f),
            ImGui::GetColorU32(color), "?");
    }
}

std::shared_ptr<Icon> resolveIcon(const std::filesystem::path& path)
{
    auto* manager = IconAccess::current();
    return manager && !path.empty() ? manager->load(path) : nullptr;
}

IconButtonMetrics measureIconContent(const std::string& label, bool hasIcon, float size)
{
    const auto& style = ImGui::GetStyle();
    const auto text = label.empty() ? ImVec2(0.0f, 0.0f) : ImGui::CalcTextSize(label.c_str());
    const auto iconSize = hasIcon ? scaledIconSize(size) : 0.0f;
    const auto gap = hasIcon && !label.empty() ? style.ItemInnerSpacing.x : 0.0f;
    const auto width = iconSize + gap + text.x;
    const auto height = std::max({ImGui::GetTextLineHeight(), text.y, iconSize});
    return {width, height, width, height, iconSize, text.x, text.y, gap};
}

IconButtonMetrics measureIconButton(const std::string& label, bool hasIcon, float size)
{
    auto metrics = measureIconContent(label, hasIcon, size);
    const auto& padding = ImGui::GetStyle().FramePadding;
    metrics.width += padding.x * 2.0f;
    metrics.height += padding.y * 2.0f;
    return metrics;
}

namespace {
void paintContent(const std::string& label, const std::shared_ptr<Icon>& icon,
    const IconButtonMetrics& metrics, float x, float y, const base::Color& tint)
{
    if (icon)
    {
        paintIcon(*icon, x, y + (metrics.contentHeight - metrics.iconSize) * 0.5f,
            metrics.iconSize, tint);
    }
    if (!label.empty())
    {
        ImGui::GetWindowDrawList()->AddText(
            ImVec2(x + metrics.iconSize + metrics.gap,
                y + (metrics.contentHeight - metrics.textHeight) * 0.5f),
            ImGui::GetColorU32(ImGuiCol_Text), label.c_str());
    }
}
}

void drawIconContent(const std::string& label, const std::shared_ptr<Icon>& icon,
    float size, const IconDrawOptions& options)
{
    if (!IconAccess::current() || (icon && !IconAccess::belongsToCurrentSession(*icon)))
    {
        return;
    }
    const auto metrics = measureIconContent(label, bool(icon), size);
    {
        DisabledScope disabled(options.disabled);
        if (options.sameLine)
        {
            ImGui::SameLine();
        }
        ImGui::Dummy(ImVec2(metrics.width, metrics.height));
        if (ImGui::IsItemVisible())
        {
            const auto min = ImGui::GetItemRectMin();
            ClipScope clip(min, ImGui::GetItemRectMax());
            paintContent(label, icon, metrics, min.x, min.y, options.tint);
        }
    }
    tooltip(options.tooltip);
}

void drawIconImage(const std::shared_ptr<Icon>& icon, float size, const IconDrawOptions& options)
{
    if (!IconAccess::current() || !icon || !IconAccess::belongsToCurrentSession(*icon))
    {
        return;
    }
    const auto boxSize = scaledIconSize(size);
    {
        DisabledScope disabled(options.disabled);
        if (options.sameLine)
        {
            ImGui::SameLine();
        }
        ImGui::Dummy(ImVec2(boxSize, boxSize));
        if (ImGui::IsItemVisible())
        {
            const auto min = ImGui::GetItemRectMin();
            ClipScope clip(min, ImGui::GetItemRectMax());
            paintIcon(*icon, min.x, min.y, boxSize, options.tint);
        }
    }
    tooltip(options.tooltip);
}

bool drawIconButton(const std::string& id, const std::string& label,
    const std::shared_ptr<Icon>& icon, float size, const IconDrawOptions& options)
{
    if (!IconAccess::current() || (icon && !IconAccess::belongsToCurrentSession(*icon)))
    {
        return false;
    }
    if (id.empty())
    {
        throw std::invalid_argument("Icon buttons require a nonempty widget ID");
    }
    const auto metrics = measureIconButton(label, bool(icon), size);
    bool pressed;
    {
        DisabledScope disabled(options.disabled);
        IdScope identity(id);
        if (options.sameLine)
        {
            ImGui::SameLine();
        }
        // A regular button supplies navigation, focus, activation and background
        // exactly once. Draw content over its blank label without new UI items.
        pressed = ImGui::Button("##icon-button", ImVec2(metrics.width, metrics.height));
        if (ImGui::IsItemVisible())
        {
            const auto min = ImGui::GetItemRectMin();
            const auto max = ImGui::GetItemRectMax();
            ClipScope clip(min, max);
            const auto& style = ImGui::GetStyle();
            const auto& align = style.ButtonTextAlign;
            const auto x = min.x + style.FramePadding.x +
                (max.x - min.x - style.FramePadding.x * 2.0f - metrics.contentWidth) * align.x;
            const auto y = min.y + style.FramePadding.y +
                (max.y - min.y - style.FramePadding.y * 2.0f - metrics.contentHeight) * align.y;
            paintContent(label, icon, metrics, x, y, options.tint);
        }
    }
    tooltip(options.tooltip);
    return pressed;
}

}
