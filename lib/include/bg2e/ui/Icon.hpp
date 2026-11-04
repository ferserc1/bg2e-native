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
#include <filesystem>
#include <memory>

namespace bg2e::ui {
namespace detail {
struct IconData;
struct IconAccess;
}

// Immutable, session-scoped icon. Source identity is independent of draw size,
// widget identity and tint. Handles become invalid when their UI is cleaned up.
class BG2E_API Icon {
  public:
    Icon(const Icon&) = delete;
    Icon& operator=(const Icon&) = delete;

    // True when this session still owns a drawable image, including a fallback.
    // False for the text-only fallback state or after UI cleanup.
    bool valid() const;
    bool isFallback() const;
    const std::filesystem::path& source() const;

  private:
    friend class IconManager;
    friend struct detail::IconAccess;
    explicit Icon(std::shared_ptr<detail::IconData> data);
    std::shared_ptr<detail::IconData> _data;
};

}
