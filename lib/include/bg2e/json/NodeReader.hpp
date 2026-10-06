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

#include <bg2e/json/JsonNode.hpp>

#include <optional>
#include <cmath>
#include <limits>
#include <type_traits>

namespace bg2e::json {

namespace detail {

struct ObjectSelector {
    using Key = std::string;

    static bool accepts(const JsonNode& node) { return node.isObject(); }

    static std::shared_ptr<JsonNode> find(const JsonNode& node, const Key& key)
    {
        const auto& values = node.objectValue();
        const auto it = values.find(key);
        return it == values.end() ? nullptr : it->second;
    }
};

struct ArraySelector {
    using Key = std::size_t;

    static bool accepts(const JsonNode& node) { return node.isList(); }

    static std::shared_ptr<JsonNode> find(const JsonNode& node, Key index)
    {
        const auto& values = node.listValue();
        return index < values.size() ? values[index] : nullptr;
    }
};

// JsonNode's vector predicates assume every list element is non-null.
inline bool numericList(const JsonNode& node, std::size_t count)
{
    if (!node.isList()) return false;
    const auto& values = node.listValue();
    if (values.size() != count) return false;
    for (const auto& value : values)
    {
        if (!value || !value->isNumber() || !std::isfinite(value->numberValue())) return false;
    }
    return true;
}

template <typename T>
std::optional<T> readValue(const std::shared_ptr<JsonNode>& node)
{
    if (!node) return std::nullopt;

    if constexpr (std::is_same_v<T, std::string>)
    {
        if (node->isString()) return node->stringValue();
    }
    else if constexpr (std::is_same_v<T, float>)
    {
        if (node->isNumber() && std::isfinite(node->numberValue())) return node->numberValue();
    }
    else if constexpr (std::is_same_v<T, bool>)
    {
        if (node->isBool()) return node->boolValue();
    }
    else if constexpr (std::is_same_v<T, std::array<float, 2>>)
    {
        if (numericList(*node, 2)) return node->vec2Value();
    }
    else if constexpr (std::is_same_v<T, std::array<float, 3>>)
    {
        if (numericList(*node, 3)) return node->vec3Value();
    }
    else if constexpr (std::is_same_v<T, std::array<float, 4>>)
    {
        if (numericList(*node, 4)) return node->vec4Value();
    }
    else if constexpr (std::is_same_v<T, std::array<float, 16>>)
    {
        if (numericList(*node, 16)) return node->mat4Value();
    }
    else if constexpr (std::is_same_v<T, glm::vec2>)
    {
        if (numericList(*node, 2)) return node->glmVec2Value();
    }
    else if constexpr (std::is_same_v<T, glm::vec3>)
    {
        if (numericList(*node, 3)) return node->glmVec3Value();
    }
    else if constexpr (std::is_same_v<T, glm::vec4>)
    {
        if (numericList(*node, 4)) return node->glmVec4Value();
    }
    else if constexpr (std::is_same_v<T, glm::mat4>)
    {
        if (numericList(*node, 16)) return node->glmMat4Value();
    }
    else if constexpr (std::is_same_v<T, base::Color>)
    {
        if (numericList(*node, 4)) return node->colorValue();
    }
    return std::nullopt;
}

} // namespace detail

template <typename Selector>
class BasicReader {
public:
    using Key = typename Selector::Key;

    explicit BasicReader(std::shared_ptr<JsonNode> node) : _node(std::move(node)) {}

    // The caller must keep a node passed by reference alive for the reader's lifetime.
    explicit BasicReader(JsonNode& node) : _node(&node, [](JsonNode*) {}) {}

    bool isValid() const { return _node && Selector::accepts(*_node); }
    const std::shared_ptr<JsonNode>& node() const { return _node; }
    std::shared_ptr<JsonNode> getNode(const Key& key) const { return find(key); }
    bool isUndefined(const Key& key) const { return !find(key); }
    bool isNull(const Key& key) const { return matches(key, &JsonNode::isNull); }
    bool isNullOrUndefined(const Key& key) const { return isUndefined(key) || isNull(key); }
    bool isObject(const Key& key) const { return matches(key, &JsonNode::isObject); }
    bool isArray(const Key& key) const { return matches(key, &JsonNode::isList); }
    bool isList(const Key& key) const { return isArray(key); }
    bool isString(const Key& key) const { return matches(key, &JsonNode::isString); }
    bool isNumber(const Key& key) const { return matches(key, &JsonNode::isNumber); }
    bool isBool(const Key& key) const { return matches(key, &JsonNode::isBool); }
    bool isVec2(const Key& key) const { return numeric(key, 2); }
    bool isVec3(const Key& key) const { return numeric(key, 3); }
    bool isVec4(const Key& key) const { return numeric(key, 4); }
    bool isColor(const Key& key) const { return isVec4(key); }
    bool isMat4(const Key& key) const { return numeric(key, 16); }

    std::optional<std::string> getString(const Key& key) const { return get<std::string>(key); }
    std::optional<float> getNumber(const Key& key) const { return get<float>(key); }
    template <typename T>
    std::optional<T> getInteger(const Key& key) const
    {
        static_assert(std::is_integral_v<T> && !std::is_same_v<T, bool>);
        auto value = getNumber(key);
        if (!value || !std::isfinite(*value) || std::trunc(*value) != *value ||
            static_cast<long double>(*value) < static_cast<long double>(std::numeric_limits<T>::min()) ||
            static_cast<long double>(*value) > static_cast<long double>(std::numeric_limits<T>::max()))
            return std::nullopt;
        return static_cast<T>(*value);
    }
    std::optional<bool> getBool(const Key& key) const { return get<bool>(key); }
    std::optional<std::array<float, 2>> getVec2(const Key& key) const { return get<std::array<float, 2>>(key); }
    std::optional<std::array<float, 3>> getVec3(const Key& key) const { return get<std::array<float, 3>>(key); }
    std::optional<std::array<float, 4>> getVec4(const Key& key) const { return get<std::array<float, 4>>(key); }
    std::optional<std::array<float, 16>> getMat4(const Key& key) const { return get<std::array<float, 16>>(key); }
    std::optional<glm::vec2> getGlmVec2(const Key& key) const { return get<glm::vec2>(key); }
    std::optional<glm::vec3> getGlmVec3(const Key& key) const { return get<glm::vec3>(key); }
    std::optional<glm::vec4> getGlmVec4(const Key& key) const { return get<glm::vec4>(key); }
    std::optional<glm::mat4> getGlmMat4(const Key& key) const { return get<glm::mat4>(key); }
    std::optional<base::Color> getColor(const Key& key) const { return get<base::Color>(key); }

    std::optional<BasicReader<detail::ObjectSelector>> getObject(const Key& key) const
    {
        auto node = find(key);
        if (!node || !node->isObject()) return std::nullopt;
        return BasicReader<detail::ObjectSelector>(std::move(node));
    }

    std::optional<BasicReader<detail::ArraySelector>> getArray(const Key& key) const
    {
        auto node = find(key);
        if (!node || !node->isList()) return std::nullopt;
        return BasicReader<detail::ArraySelector>(std::move(node));
    }

    std::optional<BasicReader<detail::ArraySelector>> getList(const Key& key) const
    {
        return getArray(key);
    }

    std::size_t size() const
    {
        if constexpr (std::is_same_v<Selector, detail::ArraySelector>)
            return isValid() ? _node->listValue().size() : 0;
        else
            return isValid() ? _node->objectValue().size() : 0;
    }

private:
    std::shared_ptr<JsonNode> find(const Key& key) const
    {
        return isValid() ? Selector::find(*_node, key) : nullptr;
    }

    bool matches(const Key& key, bool (JsonNode::*predicate)() const) const
    {
        auto node = find(key);
        return node && (node.get()->*predicate)();
    }

    bool numeric(const Key& key, std::size_t count) const
    {
        auto node = find(key);
        return node && detail::numericList(*node, count);
    }

    template <typename T>
    std::optional<T> get(const Key& key) const
    {
        return detail::readValue<T>(find(key));
    }

    std::shared_ptr<JsonNode> _node;
};

using ObjectReader = BasicReader<detail::ObjectSelector>;
using ArrayReader = BasicReader<detail::ArraySelector>;

} // namespace bg2e::json
