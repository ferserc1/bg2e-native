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
#include <any>
#include <functional>
#include <memory>
#include <string>
#include <typeindex>
#include <type_traits>
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

    // Registers a subtype and a type-safe factory for a polymorphic base.
    // Type metadata and subtype metadata may be registered in any order.
    template<typename Base, typename Derived>
    void registerSubtype(
        std::string baseTypeName,
        std::string key,
        std::string displayName,
        std::string typeName = {}
    ) {
        static_assert(std::is_base_of_v<Base, Derived>,
            "bg2e::reflection: registered subtype must derive from its base type");
        static_assert(std::is_polymorphic_v<Base>,
            "bg2e::reflection: polymorphic subtype bases must have virtual dispatch");
        static_assert(std::is_default_constructible_v<Derived>,
            "bg2e::reflection: default subtype registration requires a default constructor");
        registerSubtype<Base, Derived>(
            std::move(baseTypeName), std::move(key), std::move(displayName),
            [] { return std::make_shared<Derived>(); },
            std::move(typeName)
        );
    }

    template<typename Base, typename Derived, typename Factory>
        requires std::is_invocable_r_v<std::shared_ptr<Derived>, Factory>
    void registerSubtype(
        std::string baseTypeName,
        std::string key,
        std::string displayName,
        Factory factory,
        std::string typeName = {}
    ) {
        static_assert(std::is_base_of_v<Base, Derived>,
            "bg2e::reflection: registered subtype must derive from its base type");
        static_assert(std::is_polymorphic_v<Base>,
            "bg2e::reflection: polymorphic subtype bases must have virtual dispatch");
        SubtypeInfo info;
        info.baseTypeName = std::move(baseTypeName);
        info.key = std::move(key);
        info.typeName = typeName.empty() ? info.key : std::move(typeName);
        info.displayName = displayName.empty() ? info.key : std::move(displayName);

        RegisteredSubtype registered;
        registered.info = std::move(info);
        registered.dynamicType = std::type_index(typeid(Derived));
        registered.factory = [factory = std::move(factory)]() -> std::any {
            return std::any(std::shared_ptr<Base>(factory()));
        };
        registered.constObjectCast = [](const void * object) -> const void * {
            return dynamic_cast<const Derived*>(static_cast<const Base*>(object));
        };
        registered.mutableObjectCast = [](void * object) -> void * {
            return dynamic_cast<Derived*>(static_cast<Base*>(object));
        };
        registerSubtypeData(std::move(registered));
    }

    const SubtypeInfo * subtype(const std::string& baseTypeName, const std::string& key) const;
    std::vector<SubtypeInfo> subtypes(const std::string& baseTypeName) const;
    std::string subtypeKey(const std::string& baseTypeName, const std::type_info& dynamicType) const;
    const void * subtypeObject(
        const std::string& baseTypeName,
        const std::string& key,
        const void * baseObject
    ) const;
    void * subtypeObject(
        const std::string& baseTypeName,
        const std::string& key,
        void * baseObject
    ) const;

    template<typename Base>
    std::shared_ptr<Base> createSubtype(const std::string& baseTypeName, const std::string& key) const
    {
        const auto * registered = subtypeData(baseTypeName, key);
        if (!registered || !registered->factory) return {};
        try
        {
            return std::any_cast<std::shared_ptr<Base>>(registered->factory());
        }
        catch (const std::bad_any_cast&)
        {
            return {};
        }
    }

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

    struct RegisteredSubtype {
        SubtypeInfo info;
        std::type_index dynamicType = std::type_index(typeid(void));
        std::function<std::any()> factory;
        std::function<const void*(const void*)> constObjectCast;
        std::function<void*(void*)> mutableObjectCast;
    };

    void registerSubtypeData(RegisteredSubtype subtype);
    const RegisteredSubtype * subtypeData(
        const std::string& baseTypeName,
        const std::string& key
    ) const;

    std::unordered_map<std::string, TypeInfo> _registry;
    std::unordered_map<std::string, std::vector<RegisteredSubtype>> _subtypes;
    static TypeRegistry * _registrySingleton;
};

}
}
