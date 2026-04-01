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

#include <string>

#ifdef _WIN32

#pragma warning(disable: 4251)
#pragma warning(disable: 4275)

#ifdef _WINDLL
#define BG2E_API __declspec(dllexport)
#else
#define BG2E_API __declspec(dllimport)
#endif

#elif __APPLE__

#define BG2E_API

#else

#define BG2E_LINUX

#define BG2E_API

#include <cstdint>

#endif
