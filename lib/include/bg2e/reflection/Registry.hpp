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
#include <bg2e/reflection/TypeInfo.hpp>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace bg2e {
namespace reflection {

class BG2E_API TypeRegistry {
public:
    static TypeRegistry& get();

    // Registers (or replaces) the metadata for info.typeName.
    void registerType(TypeInfo info);

    // nullptr if the type has no reflection metadata.
    const TypeInfo * type(const std::string& typeName) const;
    bool contains(const std::string& typeName) const;
    std::vector<std::string> typeNames() const;

    // Depth of the deepest Object-property chain reachable from typeName.
    // 0 = the type has no object properties (or is not registered).
    // Unregistered objectTypeName keys and reference cycles stop the chain
    // (a visited set guards against cycles, which by-value nesting cannot
    // produce but future pointer-based nesting could).
    uint32_t objectChainDepth(const std::string& typeName) const;

    // True if every registered type respects maxObjectDepth. When
    // 'offenders' is non-null it is filled with the names of the types
    // whose chains exceed the limit.
    bool validateObjectDepth(std::vector<std::string>* offenders = nullptr) const;

protected:
    TypeRegistry() = default;

    std::unordered_map<std::string, TypeInfo> _registry;
    static TypeRegistry * _registrySingleton;
};

}
}
