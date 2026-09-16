/*
 *    business grade graphic engine (bg2 engine)
 *    Copyright (C) 2026  Fernando Serrano Carpena
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 */

#include <bg2e/ui/ResourcePicker.hpp>
#include <bg2e/app/FileDialog.hpp>
#include <bg2e/ui/Button.hpp>
#include <bg2e/ui/Group.hpp>
#include <bg2e/ui/Layout.hpp>
#include <bg2e/ui/Text.hpp>
#include <bg2e/ui/Value.hpp>

#include "imgui.h"

#include <algorithm>
#include <system_error>

namespace bg2e::ui {

namespace {

std::string extensionSpec(const std::vector<std::string> & extensions)
{
    std::string result;
    for (auto extension : extensions)
    {
        while (!extension.empty() && (extension.front() == '.' || extension.front() == '*'))
        {
            extension.erase(extension.begin());
        }
        if (extension.empty()) continue;
        if (!result.empty()) result += ',';
        result += extension;
    }
    return result.empty() ? "*" : result;
}

} // anonymous namespace

bool ResourcePicker::draw(
    const std::string & label,
    std::filesystem::path & value,
    const reflection::PropertyMetadata & metadata,
    bool readOnly
) {
    if (readOnly) Group::beginDisabled();

    // The label goes in the header instead of on the left of the input.
    // The label parameter may carry a "##id" suffix: only the visible
    // part is drawn.
    const auto visibleLabel = label.substr(0, label.find("##"));
    Text::text(visibleLabel);

    bool changed = false;

    // Non-editable field showing only the file name (last path component),
    // followed by the browse button on the same line
    auto fileName = value.filename().string();
    const auto textCapacity = std::max(1024, static_cast<int>(fileName.size()) + 256);
    const auto browseWidth = static_cast<float>(
        Layout::calcButtonWidth("Browse") + Layout::getItemHorizontalSpacing()
    );
    ImGui::SetNextItemWidth(Layout::getContentRegionAvailWidth() - browseWidth);
    Value::text(label, fileName, true, textCapacity);

    if (Button::button("Browse##" + label, true))
    {
        app::FileDialog dialog;
        const auto filterName = metadata.resourceKind.empty() ? "Resources" : metadata.resourceKind;
        dialog.setFilters({ { filterName, extensionSpec(metadata.resourceExtensions) } });
        auto selected = dialog.openFile();
        if (!selected.empty())
        {
            if (metadata.resourcePathIsProjectRelative)
            {
                std::error_code error;
                auto relative = std::filesystem::relative(selected, std::filesystem::current_path(), error);
                value = error ? selected : relative;
            }
            else
            {
                value = selected;
            }
            changed = true;
        }
    }

    if (readOnly) Group::endDisabled();
    return changed && !readOnly;
}

}
