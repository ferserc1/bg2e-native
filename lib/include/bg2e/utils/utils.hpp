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
namespace utils {

/**
    uniqueId: generate an unique identifier in the same format as the RFC 4122, but this
    is NOT an RFC 4122 uuid. Is a quick method to generate identifiers that are unique in
    the local device. These identifiers have been created to share the same format as a
    UUID in case in the future you want to replace the algorithm to generate real UUIDs.
 */
extern BG2E_API std::string uniqueId();

}
}
