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

#include <bg2e/draw/DrawableCache.hpp>

namespace bg2e::scene {
class Component;
class Node;

// Active contexts reachable from the document, including bound nested scopes.
// Engine-global bookkeeping is deliberately not a document protection root.
BG2E_API void collectDrawableCacheContexts(const Node& root,
    std::vector<std::shared_ptr<draw::DrawableCache>>& contexts);

// Explicit history/job lifetime pins. They keep the device-backed context and
// current realization alive without inventing a second owner or using refcounts.
struct [[nodiscard]] DrawableResourceRetention {
    std::vector<draw::DrawableResourceHandle> identities;
    std::vector<draw::DrawableUsageLease> leases;
    draw::DrawableCacheDiagnostic diagnostic;
};

BG2E_API DrawableResourceRetention retainDrawableResources(const Component& component,
    draw::DrawableUse use, const std::string& debugRole);
BG2E_API DrawableResourceRetention retainDrawableResources(const Node& root,
    draw::DrawableUse use, const std::string& debugRole);

}
