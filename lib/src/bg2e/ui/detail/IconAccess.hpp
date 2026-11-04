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

#include <memory>

namespace bg2e::ui {
class Icon;
class IconManager;

namespace detail {
struct ImageEntry;

// Private bridge for path-based widgets. Never owns a manager or a session.
struct IconAccess {
    static IconManager* current();
    static bool belongsToCurrentSession(const Icon& icon);
    static std::shared_ptr<ImageEntry> image(const Icon& icon);
};

}
}
