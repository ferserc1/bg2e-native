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

namespace bg2e::ui {

// Numeric values are stable: persisted in the ui preferences context.
enum class ToolbarPresentation
{
    Text = 0,
    IconsWithTooltips = 1,
    IconsAndText = 2
};

struct ToolbarConfiguration {
    // Supplying an icon path is enough to display it by default. Entries
    // without paths stay textual; an explicit Text preference still wins.
    ToolbarPresentation presentation = ToolbarPresentation::IconsAndText;
    float iconSize = 16.0f;
};

}
