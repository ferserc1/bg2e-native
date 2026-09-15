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

#include <bg2e/reflection/Registry.hpp>

#include <unordered_set>
#include <utility>

namespace bg2e::reflection {

namespace {

// DFS over object-property edges. 'visited' holds the type names on the
// current path: it cuts reference cycles and is undone on the way out so
// shared sub-chains (diamonds) still report accurate depths.
uint32_t objectChainDepthFrom(const TypeRegistry& registry,
                              const std::string& typeName,
                              std::unordered_set<std::string>& visited)
{
    if (!visited.insert(typeName).second)
    {
        return 0;
    }

    uint32_t maxDepth = 0;
    if (const TypeInfo * info = registry.type(typeName))
    {
        for (const PropertyInfo& p : info->properties)
        {
            if (p.type == PropertyType::Object)
            {
                // Unregistered keys terminate the chain (they contribute
                // 0 further depth); reflection is optional by design.
                uint32_t depth = 1 + objectChainDepthFrom(registry, p.objectTypeName, visited);
                if (depth > maxDepth)
                {
                    maxDepth = depth;
                }
            }
        }
    }

    visited.erase(typeName);
    return maxDepth;
}

} // namespace

TypeRegistry * TypeRegistry::_registrySingleton = nullptr;

TypeRegistry& TypeRegistry::get()
{
    if (_registrySingleton == nullptr)
    {
        _registrySingleton = new TypeRegistry();
    }
    return *_registrySingleton;
}

void TypeRegistry::registerType(TypeInfo info)
{
    _registry[info.typeName] = std::move(info);
}

const TypeInfo * TypeRegistry::type(const std::string& typeName) const
{
    auto it = _registry.find(typeName);
    return it != _registry.end() ? &it->second : nullptr;
}

bool TypeRegistry::contains(const std::string& typeName) const
{
    return _registry.find(typeName) != _registry.end();
}

std::vector<std::string> TypeRegistry::typeNames() const
{
    std::vector<std::string> result;
    result.reserve(_registry.size());
    for (const auto& [name, _] : _registry)
    {
        result.push_back(name);
    }
    return result;
}

void TypeRegistry::registerSubtypeData(RegisteredSubtype subtype)
{
    auto & entries = _subtypes[subtype.info.baseTypeName];
    for (auto & entry : entries)
    {
        if (entry.info.key == subtype.info.key)
        {
            entry = std::move(subtype);
            return;
        }
    }
    entries.push_back(std::move(subtype));
}

const TypeRegistry::RegisteredSubtype * TypeRegistry::subtypeData(
    const std::string& baseTypeName,
    const std::string& key
) const {
    auto hierarchy = _subtypes.find(baseTypeName);
    if (hierarchy == _subtypes.end()) return nullptr;
    for (const auto & entry : hierarchy->second)
    {
        if (entry.info.key == key) return &entry;
    }
    return nullptr;
}

const SubtypeInfo * TypeRegistry::subtype(
    const std::string& baseTypeName,
    const std::string& key
) const {
    const auto * entry = subtypeData(baseTypeName, key);
    return entry ? &entry->info : nullptr;
}

std::vector<SubtypeInfo> TypeRegistry::subtypes(const std::string& baseTypeName) const
{
    std::vector<SubtypeInfo> result;
    auto hierarchy = _subtypes.find(baseTypeName);
    if (hierarchy == _subtypes.end()) return result;
    result.reserve(hierarchy->second.size());
    for (const auto & entry : hierarchy->second)
    {
        result.push_back(entry.info);
    }
    return result;
}

std::string TypeRegistry::subtypeKey(
    const std::string& baseTypeName,
    const std::type_info& dynamicType
) const {
    auto hierarchy = _subtypes.find(baseTypeName);
    if (hierarchy == _subtypes.end()) return {};
    const std::type_index type(dynamicType);
    for (const auto & entry : hierarchy->second)
    {
        if (entry.dynamicType == type) return entry.info.key;
    }
    return {};
}

const void * TypeRegistry::subtypeObject(
    const std::string& baseTypeName,
    const std::string& key,
    const void * baseObject
) const {
    const auto * entry = subtypeData(baseTypeName, key);
    return entry && entry->constObjectCast && baseObject
        ? entry->constObjectCast(baseObject)
        : nullptr;
}

void * TypeRegistry::subtypeObject(
    const std::string& baseTypeName,
    const std::string& key,
    void * baseObject
) const {
    const auto * entry = subtypeData(baseTypeName, key);
    return entry && entry->mutableObjectCast && baseObject
        ? entry->mutableObjectCast(baseObject)
        : nullptr;
}

uint32_t TypeRegistry::objectChainDepth(const std::string& typeName) const
{
    std::unordered_set<std::string> visited;
    return objectChainDepthFrom(*this, typeName, visited);
}

bool TypeRegistry::validateObjectDepth(std::vector<std::string>* offenders) const
{
    bool ok = true;
    for (const std::string& name : typeNames())
    {
        if (objectChainDepth(name) > maxObjectDepth)
        {
            ok = false;
            if (offenders)
            {
                offenders->push_back(name);
            }
        }
    }
    return ok;
}

}
