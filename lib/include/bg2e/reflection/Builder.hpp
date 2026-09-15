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

#include <bg2e/reflection/Registry.hpp>
#include <bg2e/math/base.hpp>
#include <bg2e/base/Color.hpp>

#include <any>
#include <cstdint>
#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>

namespace bg2e {
namespace reflection {

template<typename T>
using ValueT = std::remove_cv_t<std::remove_reference_t<T>>;

template<typename Class, typename MemberFn>
struct GetterTraits;

template<typename Class, typename R>
struct GetterTraits<Class, R (Class::*)() const> {
    using ValueType = ValueT<R>;
};

template<typename Class, typename R>
struct GetterTraits<Class, R (Class::*)()> {
    using ValueType = ValueT<R>;
};

template<typename Class, typename MemberFn>
struct SetterTraits;

template<typename Class, typename V>
struct SetterTraits<Class, void (Class::*)(V)> {
    using ValueType = ValueT<V>;
};

namespace detail {

template<typename T>
constexpr std::optional<PropertyType> scalarPropertyTypeOf()
{
    using U = ValueT<T>;
    if constexpr (std::is_same_v<U, bool>)                  return PropertyType::Bool;
    else if constexpr (std::is_same_v<U, int32_t>)          return PropertyType::Int;
    else if constexpr (std::is_same_v<U, uint32_t>)         return PropertyType::UInt;
    else if constexpr (std::is_same_v<U, float>)            return PropertyType::Float;
    else if constexpr (std::is_same_v<U, double>)           return PropertyType::Double;
    else if constexpr (std::is_same_v<U, std::string>)      return PropertyType::String;
    else if constexpr (std::is_same_v<U, glm::vec2>)        return PropertyType::Vec2;
    else if constexpr (std::is_same_v<U, glm::vec3>)        return PropertyType::Vec3;
    else if constexpr (std::is_same_v<U, glm::vec4>)        return PropertyType::Vec4;
    else if constexpr (std::is_same_v<U, glm::mat4>)        return PropertyType::Mat4;
    else if constexpr (std::is_same_v<U, base::Color>)      return PropertyType::Color;
    else if constexpr (std::is_same_v<U, std::filesystem::path>) return PropertyType::Path;
    else if constexpr (std::is_enum_v<U>)                   return PropertyType::Enum;
    else                                                    return std::nullopt;
}

} // namespace detail

template<typename T>
constexpr bool isScalarPropertyType()
{
    return detail::scalarPropertyTypeOf<T>().has_value();
}

template<typename T>
constexpr PropertyType propertyTypeOf()
{
    constexpr auto type = detail::scalarPropertyTypeOf<T>();
    static_assert(type.has_value(), "bg2e::reflection: unsupported property type");
    return *type;
}

template<typename T> class TypeInfoBuilder;

template<typename T>
class PropertyBuilder {
public:
    PropertyBuilder(TypeInfoBuilder<T>& builder, size_t index)
        : _builder(builder), _index(index) {}

    PropertyBuilder& displayName(std::string v) { info().metadata.displayName = std::move(v); return *this; }
    PropertyBuilder& category(std::string v)    { info().metadata.category = std::move(v); return *this; }
    PropertyBuilder& tooltip(std::string v)     { info().metadata.tooltip = std::move(v); return *this; }

    PropertyBuilder& range(double minV, double maxV) { info().metadata.min = minV; info().metadata.max = maxV; return *this; }
    PropertyBuilder& min(double v)  { info().metadata.min = v; return *this; }
    PropertyBuilder& max(double v)  { info().metadata.max = v; return *this; }
    PropertyBuilder& step(double v) { info().metadata.step = v; return *this; }

    PropertyBuilder& editor(PropertyEditor e) { info().editor = e; return *this; }
    PropertyBuilder& input()    { return editor(PropertyEditor::Input); }
    PropertyBuilder& slider()   { return editor(PropertyEditor::Slider); }
    PropertyBuilder& drag()     { return editor(PropertyEditor::Drag); }
    PropertyBuilder& checkbox() { return editor(PropertyEditor::Checkbox); }
    PropertyBuilder& combo()    { return editor(PropertyEditor::Combo); }
    PropertyBuilder& colorEditor() { return editor(PropertyEditor::Color); }
    PropertyBuilder& angle()    { return editor(PropertyEditor::Angle); }

    PropertyBuilder& enumValue(std::string label, int64_t value)
    {
        info().metadata.enumOptions.emplace_back(std::move(label), value);
        return *this;
    }

    template<typename EnumT>
    PropertyBuilder& enumValue(std::string label, EnumT value)
    {
        return enumValue(std::move(label), static_cast<int64_t>(value));
    }

private:
    PropertyInfo& info() { return _builder.propertyAt(_index); }

    TypeInfoBuilder<T>& _builder;
    size_t _index;
};

template<typename T>
class ObjectBuilder {
public:
    ObjectBuilder(TypeInfoBuilder<T>& builder, size_t index)
        : _builder(builder), _index(index) {}

    ObjectBuilder& displayName(std::string v) { info().metadata.displayName = std::move(v); return *this; }
    ObjectBuilder& category(std::string v)    { info().metadata.category = std::move(v); return *this; }
    ObjectBuilder& tooltip(std::string v)     { info().metadata.tooltip = std::move(v); return *this; }

    // No editor(), slider(), drag(), range(), min(), max(), step(),
    // enumValue(), ... : they are meaningless for object properties and
    // must fail to compile instead of being silently ignored.

private:
    PropertyInfo& info() { return _builder.propertyAt(_index); }

    TypeInfoBuilder<T>& _builder;
    size_t _index;
};

template<typename T>
class ActionBuilder {
public:
    ActionBuilder(TypeInfoBuilder<T>& builder, size_t index)
        : _builder(builder), _index(index) {}

    ActionBuilder& displayName(std::string v) { info().displayName = std::move(v); return *this; }
    ActionBuilder& category(std::string v)    { info().category = std::move(v); return *this; }
    ActionBuilder& tooltip(std::string v)     { info().tooltip = std::move(v); return *this; }

private:
    ActionInfo& info() { return _builder.actionAt(_index); }

    TypeInfoBuilder<T>& _builder;
    size_t _index;
};

template<typename T>
class TypeInfoBuilder {
public:
    explicit TypeInfoBuilder(std::string typeName)
    {
        _info.typeName = std::move(typeName);
    }

    TypeInfoBuilder& displayName(std::string name)
    {
        _info.displayName = std::move(name);
        return *this;
    }

    template<typename Getter, typename Setter>
    PropertyBuilder<T> property(std::string name, Getter getter, Setter setter)
    {
        using GetterValue = typename GetterTraits<T, Getter>::ValueType;
        using SetterValue = typename SetterTraits<T, Setter>::ValueType;
        static_assert(std::is_same_v<GetterValue, SetterValue>,
            "bg2e::reflection: getter and setter must use the same value type");

        PropertyInfo p;
        p.name = std::move(name);
        p.type = propertyTypeOf<GetterValue>();
        p.getter = [getter](const void * instance) -> std::any {
            auto object = const_cast<T*>(static_cast<const T*>(instance));
            return std::any((object->*getter)());
        };
        p.setter = [setter](void * instance, const std::any& value) {
            (static_cast<T*>(instance)->*setter)(std::any_cast<GetterValue>(value));
        };
        _info.properties.push_back(std::move(p));
        return PropertyBuilder<T>(*this, _info.properties.size() - 1);
    }

    template<typename Getter>
    PropertyBuilder<T> property(std::string name, Getter getter)
    {
        using GetterValue = typename GetterTraits<T, Getter>::ValueType;

        PropertyInfo p;
        p.name = std::move(name);
        p.type = propertyTypeOf<GetterValue>();
        p.getter = [getter](const void * instance) -> std::any {
            auto object = const_cast<T*>(static_cast<const T*>(instance));
            return std::any((object->*getter)());
        };
        _info.properties.push_back(std::move(p));
        return PropertyBuilder<T>(*this, _info.properties.size() - 1);
    }

    // Read-only object property: const reference getter only.
    // Getter signature: const U& (T::*)() const
    template<typename Getter>
    ObjectBuilder<T> object(std::string name, std::string objectTypeName, Getter getter)
    {
        using U = typename GetterTraits<T, Getter>::ValueType;
        static_assert(std::is_class_v<U> && !isScalarPropertyType<U>(),
            "bg2e::reflection: object() requires a non-scalar class type; use property() for scalars");

        PropertyInfo p;
        p.name = std::move(name);
        p.type = PropertyType::Object;
        p.objectTypeName = std::move(objectTypeName);
        p.objectGetter = [getter](const void * instance) -> const void * {
            auto object = static_cast<const T*>(instance);
            return static_cast<const void*>(&(object->*getter)());
        };
        _info.properties.push_back(std::move(p));
        return ObjectBuilder<T>(*this, _info.properties.size() - 1);
    }

    // Editable object property: const + mutable reference getters.
    // Signatures: const U& (T::*)() const  and  U& (T::*)()
    template<typename ConstGetter, typename MutableGetter>
    ObjectBuilder<T> object(std::string name, std::string objectTypeName,
                            ConstGetter constGetter, MutableGetter mutableGetter)
    {
        using U = typename GetterTraits<T, ConstGetter>::ValueType;
        using MU = typename GetterTraits<T, MutableGetter>::ValueType;
        static_assert(std::is_same_v<U, MU>,
            "bg2e::reflection: const and mutable object getters must return the same type");
        static_assert(std::is_class_v<U> && !isScalarPropertyType<U>(),
            "bg2e::reflection: object() requires a non-scalar class type; use property() for scalars");

        PropertyInfo p;
        p.name = std::move(name);
        p.type = PropertyType::Object;
        p.objectTypeName = std::move(objectTypeName);
        p.objectGetter = [constGetter](const void * instance) -> const void * {
            auto object = static_cast<const T*>(instance);
            return static_cast<const void*>(&(object->*constGetter)());
        };
        p.objectMutableGetter = [mutableGetter](void * instance) -> void * {
            auto object = static_cast<T*>(instance);
            return static_cast<void*>(&(object->*mutableGetter)());
        };
        _info.properties.push_back(std::move(p));
        return ObjectBuilder<T>(*this, _info.properties.size() - 1);
    }

    template<typename Method>
    ActionBuilder<T> action(std::string name, Method method)
    {
        ActionInfo a;
        a.name = std::move(name);
        a.invoke = [method](void * instance) {
            (static_cast<T*>(instance)->*method)();
        };
        _info.actions.push_back(std::move(a));
        return ActionBuilder<T>(*this, _info.actions.size() - 1);
    }

    const TypeInfo& build() const { return _info; }

    PropertyInfo& propertyAt(size_t index) { return _info.properties[index]; }
    ActionInfo& actionAt(size_t index)     { return _info.actions[index]; }

protected:
    TypeInfo _info;
};

}
}
