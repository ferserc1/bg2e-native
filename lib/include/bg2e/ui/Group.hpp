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

#include <string>

namespace bg2e {
namespace ui {

// Scoped helpers that group widgets or modify the state of a group of
// subsequent widgets (all begin/end or push/pop pairs)
class BG2E_API Group {
public:
    static bool beginTree(const std::string & label);
    static void endTree();
    // allowOverlap lets widgets drawn on top of the header row (via
    // Layout::sameLine) receive mouse clicks instead of the header.
    static bool collapsingHeader(const std::string & title, bool visible = true, bool allowOverlap = false);

    // Renders the widgets between beginDisabled()/endDisabled() as disabled.
    // endDisabled() must always be called after beginDisabled().
    static void beginDisabled(bool disabled = true);
    static void endDisabled();

    // Pushes/pops an identifier into the ImGui ID stack to disambiguate
    // repeated labels. popId() must always be called after pushId().
    static void pushId(int id);
    static void popId();
};

}
}
