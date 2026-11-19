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
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

namespace bg2e::draw {

namespace detail {
struct DrawableCacheEntry;
struct DrawableUsageRecord;
}

class DrawableCache;

// Non-owning identity. Copying a handle does not register a scene/history use.
// Handles survive replacement, but expire on explicit unload or engine cleanup.
class BG2E_API DrawableResourceHandle {
  public:
    DrawableResourceHandle() = default;

    bool valid() const noexcept;
    const std::string& identifier() const noexcept;
    uint64_t geometryRevision() const noexcept;
    std::shared_ptr<DrawableCache> context() const noexcept;
    bool operator==(const DrawableResourceHandle& other) const noexcept;

  private:
    friend class DrawableCache;
    std::string _identifier;
    std::weak_ptr<detail::DrawableCacheEntry> _entry;
};

enum class DrawableUse
{
    SceneOwner,
    SceneInstance,
    History,
    Job,
    Frame
};

// Logical registrations, never shared_ptr reference counts or disk accounting.
struct DrawableUsage {
    bool registered = false;
    size_t sceneOwners = 0;
    size_t sceneInstances = 0;
    size_t history = 0;
    size_t jobs = 0;
    size_t frames = 0;

    bool unused() const noexcept
    {
        return sceneOwners == 0 && sceneInstances == 0 && history == 0 && jobs == 0 && frames == 0;
    }
};

}
