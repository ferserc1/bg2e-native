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

#include <cstdint>
#include <limits>

namespace bg2e::ui {

using WorkspaceMask = std::uint32_t;

inline constexpr WorkspaceMask Workspace0 = WorkspaceMask{1} << 0;
inline constexpr WorkspaceMask Workspace1 = WorkspaceMask{1} << 1;
inline constexpr WorkspaceMask Workspace2 = WorkspaceMask{1} << 2;
inline constexpr WorkspaceMask Workspace3 = WorkspaceMask{1} << 3;
inline constexpr WorkspaceMask Workspace4 = WorkspaceMask{1} << 4;
inline constexpr WorkspaceMask Workspace5 = WorkspaceMask{1} << 5;
inline constexpr WorkspaceMask Workspace6 = WorkspaceMask{1} << 6;
inline constexpr WorkspaceMask Workspace7 = WorkspaceMask{1} << 7;
inline constexpr WorkspaceMask Workspace8 = WorkspaceMask{1} << 8;
inline constexpr WorkspaceMask Workspace9 = WorkspaceMask{1} << 9;
inline constexpr WorkspaceMask Workspace10 = WorkspaceMask{1} << 10;
inline constexpr WorkspaceMask Workspace11 = WorkspaceMask{1} << 11;
inline constexpr WorkspaceMask Workspace12 = WorkspaceMask{1} << 12;
inline constexpr WorkspaceMask Workspace13 = WorkspaceMask{1} << 13;
inline constexpr WorkspaceMask Workspace14 = WorkspaceMask{1} << 14;
inline constexpr WorkspaceMask Workspace15 = WorkspaceMask{1} << 15;
inline constexpr WorkspaceMask Workspace16 = WorkspaceMask{1} << 16;
inline constexpr WorkspaceMask Workspace17 = WorkspaceMask{1} << 17;
inline constexpr WorkspaceMask Workspace18 = WorkspaceMask{1} << 18;
inline constexpr WorkspaceMask Workspace19 = WorkspaceMask{1} << 19;
inline constexpr WorkspaceMask Workspace20 = WorkspaceMask{1} << 20;
inline constexpr WorkspaceMask Workspace21 = WorkspaceMask{1} << 21;
inline constexpr WorkspaceMask Workspace22 = WorkspaceMask{1} << 22;
inline constexpr WorkspaceMask Workspace23 = WorkspaceMask{1} << 23;
inline constexpr WorkspaceMask Workspace24 = WorkspaceMask{1} << 24;
inline constexpr WorkspaceMask Workspace25 = WorkspaceMask{1} << 25;
inline constexpr WorkspaceMask Workspace26 = WorkspaceMask{1} << 26;
inline constexpr WorkspaceMask Workspace27 = WorkspaceMask{1} << 27;
inline constexpr WorkspaceMask Workspace28 = WorkspaceMask{1} << 28;
inline constexpr WorkspaceMask Workspace29 = WorkspaceMask{1} << 29;
inline constexpr WorkspaceMask Workspace30 = WorkspaceMask{1} << 30;
inline constexpr WorkspaceMask Workspace31 = WorkspaceMask{1} << 31;
inline constexpr WorkspaceMask WorkspaceAll = std::numeric_limits<WorkspaceMask>::max();

}
