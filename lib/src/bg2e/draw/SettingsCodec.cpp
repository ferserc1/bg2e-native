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

#include <bg2e/draw/SettingsCodec.hpp>
#include <bg2e/math/base.hpp>
#include <bg2e/base/Color.hpp>
#include <algorithm>
#include <type_traits>
#include <charconv>
#include <cmath>
#include <fstream>
#include <iterator>
#include <limits>
#include <set>
#include <stdexcept>

namespace bg2e::draw {
namespace {
using Node = json::JsonNode;
using Ptr = std::shared_ptr<Node>;
using Type = reflection::PropertyType;
const Node& field(const Node& node, const std::string& name)
{
    if (!node.isObject())
    {
        throw std::invalid_argument("Expected a settings JSON object");
    }
    const auto found = node.objectValue().find(name);
    if (found == node.objectValue().end() || !found->second)
    {
        throw std::invalid_argument("Missing settings field: " + name);
    }
    return *found->second;
}
std::string text(const Node& node)
{
    if (!node.isString())
    {
        throw std::invalid_argument("Expected a settings string, not a coerced JSON value");
    }
    return node.stringValue();
}
std::string typeName(Type type)
{
    switch (type)
    {
    case Type::Bool: return "bool";
    case Type::Int: return "int32";
    case Type::UInt: return "uint32";
    case Type::Float: return "float";
    case Type::Double: return "double";
    case Type::String: return "string";
    case Type::Path: return "path";
    case Type::Enum: return "enum64";
    case Type::Vec2: return "vec2";
    case Type::Vec3: return "vec3";
    case Type::Vec4: return "vec4";
    case Type::Mat4: return "mat4";
    case Type::Color: return "color";
    default: throw std::invalid_argument("Unsupported settings property type");
    }
}
Type storageType(const reflection::PropertyInfo& p)
{
    return p.type == Type::Resource ? p.metadata.resourceValueType : p.type;
}
template <typename T>
std::string decimal(T value)
{
    char buffer[128];
    std::to_chars_result result;
    if constexpr (std::is_floating_point_v<T>)
    {
        result = std::to_chars(buffer, buffer + sizeof(buffer), value, std::chars_format::general, std::numeric_limits<T>::max_digits10);
    }
    else
    {
        result = std::to_chars(buffer, buffer + sizeof(buffer), value);
    }
    if (result.ec != std::errc{})
    {
        throw std::invalid_argument("Setting number cannot be represented");
    }
    return {buffer, result.ptr};
}
template <typename T>
T numeric(const Node& node)
{
    const auto input = text(node);
    T value{};
    auto result = std::from_chars(input.data(), input.data() + input.size(), value);
    if (input.empty() || result.ec != std::errc{} || result.ptr != input.data() + input.size())
    {
        throw std::invalid_argument("Malformed or overflowing typed settings number");
    }
    if constexpr (std::is_floating_point_v<T>)
    {
        if (!std::isfinite(value))
        {
            throw std::invalid_argument("Settings numbers must be finite");
        }
    }
    return value;
}
Ptr encode(const reflection::PropertyInfo& p, const std::any& value)
{
    SettingsCatalog::validateValue(p, value);
    switch (storageType(p))
    {
    case Type::Bool: return json::JSON(std::any_cast<bool>(value));
    case Type::Int: return json::JSON(decimal(std::any_cast<int32_t>(value)));
    case Type::UInt: return json::JSON(decimal(std::any_cast<uint32_t>(value)));
    case Type::Float: return json::JSON(decimal(std::any_cast<float>(value)));
    case Type::Double: return json::JSON(decimal(std::any_cast<double>(value)));
    case Type::Enum: return json::JSON(decimal(std::any_cast<int64_t>(value)));
    case Type::String: return json::JSON(std::any_cast<std::string>(value));
    case Type::Path:
    {
        const auto utf8 = std::any_cast<std::filesystem::path>(value).generic_u8string();
        return json::JSON(std::string(utf8.begin(), utf8.end()));
    }
    default: break;
    }
    json::JsonList list;
    const auto append = [&](float v)
    {
        list.push_back(json::JSON(decimal(v)));
    };
    if (p.type == Type::Mat4)
    {
        const auto matrix = std::any_cast<glm::mat4>(value);
        for (int c = 0; c < 4; ++c)
        {
            for (int row = 0; row < 4; ++row)
            {
                append(matrix[c][row]);
            }
        }
    }
    else if (p.type == Type::Color)
    {
        const auto color = std::any_cast<base::Color>(value);
        for (float v : {color.r, color.g, color.b, color.a})
        {
            append(v);
        }
    }
    else if (p.type == Type::Vec2)
    {
        const auto v = std::any_cast<glm::vec2>(value);
        for (int i = 0; i < 2; ++i)
        {
            append(v[i]);
        }
    }
    else if (p.type == Type::Vec3)
    {
        const auto v = std::any_cast<glm::vec3>(value);
        for (int i = 0; i < 3; ++i)
        {
            append(v[i]);
        }
    }
    else if (p.type == Type::Vec4)
    {
        const auto v = std::any_cast<glm::vec4>(value);
        for (int i = 0; i < 4; ++i)
        {
            append(v[i]);
        }
    }
    else
    {
        throw std::invalid_argument("Unsupported settings value type");
    }
    return json::JSON(std::move(list));
}
std::any decode(const std::string& type, const Node& value)
{
    if (type == "bool")
    {
        if (!value.isBool())
        {
            throw std::invalid_argument("Expected a JSON boolean setting");
        }
        return value.boolValue();
    }
    if (type == "int32")
    {
        return numeric<int32_t>(value);
    }
    if (type == "uint32")
    {
        return numeric<uint32_t>(value);
    }
    if (type == "enum64")
    {
        return numeric<int64_t>(value);
    }
    if (type == "float")
    {
        return numeric<float>(value);
    }
    if (type == "double")
    {
        return numeric<double>(value);
    }
    if (type == "string")
    {
        return text(value);
    }
    if (type == "path")
    {
        const auto utf8 = text(value);
        return std::filesystem::path(std::u8string(utf8.begin(), utf8.end()));
    }
    const size_t count = type == "vec2" ? 2 : type == "vec3" ? 3 : type == "vec4" || type == "color" ? 4 : type == "mat4" ? 16 : 0;
    if (!count || !value.isList() || value.listValue().size() != count)
    {
        throw std::invalid_argument("Unknown type or malformed numeric settings array");
    }
    float values[16]{};
    for (size_t i = 0; i < count; ++i)
    {
        if (!value.listValue()[i])
        {
            throw std::invalid_argument("Null settings array item");
        }
        values[i] = numeric<float>(*value.listValue()[i]);
    }
    if (type == "vec2")
    {
        return glm::vec2(values[0], values[1]);
    }
    if (type == "vec3")
    {
        return glm::vec3(values[0], values[1], values[2]);
    }
    if (type == "vec4")
    {
        return glm::vec4(values[0], values[1], values[2], values[3]);
    }
    if (type == "color")
    {
        return base::Color(values[0], values[1], values[2], values[3]);
    }
    glm::mat4 matrix{1};
    for (int c = 0; c < 4; ++c)
    {
        for (int row = 0; row < 4; ++row)
        {
            matrix[c][row] = values[c * 4 + row];
        }
    }
    return matrix;
}
// Strict parser for the typed settings wire grammar. It deliberately rejects
// ordinary number tokens before JsonNode can round them into its float storage.
class Reader {
  public:
    explicit Reader(const std::string& text) : _text(text)
    {
    }
    Ptr parse()
    {
        auto result = value(0);
        space();
        if (_position != _text.size())
        {
            throw std::invalid_argument("Trailing settings JSON content");
        }
        return result;
    }
  private:
    const std::string& _text;
    size_t _position = 0;
    void space()
    {
        while (_position < _text.size() && (_text[_position] == ' ' || _text[_position] == '\n' || _text[_position] == '\r' || _text[_position] == '\t'))
        {
            ++_position;
        }
    }
    bool take(char c)
    {
        space();
        if (_position < _text.size() && _text[_position] == c)
        {
            ++_position;
            return true;
        }
        return false;
    }
    void require(char c)
    {
        if (!take(c))
        {
            throw std::invalid_argument("Malformed settings JSON punctuation");
        }
    }
    uint32_t hex4()
    {
        uint32_t v = 0;
        for (int i = 0; i < 4; ++i)
        {
            if (_position == _text.size())
            {
                throw std::invalid_argument("Truncated Unicode escape");
            }
            const char c = _text[_position++];
            const int digit = c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10 : c >= 'A' && c <= 'F' ? c - 'A' + 10 : -1;
            if (digit < 0)
            {
                throw std::invalid_argument("Invalid Unicode escape");
            }
            v = v * 16 + uint32_t(digit);
        }
        return v;
    }
    static void utf8(std::string& out, uint32_t cp)
    {
        if (cp < 0x80)
        {
            out += char(cp);
        }
        else if (cp < 0x800)
        {
            out += char(0xc0 | (cp >> 6));
            out += char(0x80 | (cp & 63));
        }
        else if (cp < 0x10000)
        {
            out += char(0xe0 | (cp >> 12));
            out += char(0x80 | ((cp >> 6) & 63));
            out += char(0x80 | (cp & 63));
        }
        else
        {
            out += char(0xf0 | (cp >> 18));
            out += char(0x80 | ((cp >> 12) & 63));
            out += char(0x80 | ((cp >> 6) & 63));
            out += char(0x80 | (cp & 63));
        }
    }
    std::string string()
    {
        require('"');
        std::string out;
        while (_position < _text.size())
        {
            const auto c = uint8_t(_text[_position++]);
            if (c == '"')
            {
                return out;
            }
            if (c < 32)
            {
                throw std::invalid_argument("Unescaped control character in settings JSON");
            }
            if (c != '\\')
            {
                if (c < 128)
                {
                    out += char(c);
                    continue;
                }
                const unsigned count = c >= 0xc2 && c <= 0xdf ? 1 : c >= 0xe0 && c <= 0xef ? 2 : c >= 0xf0 && c <= 0xf4 ? 3 : 0;
                uint32_t cp = c & (count == 1 ? 31u : count == 2 ? 15u : 7u);
                if (!count)
                {
                    throw std::invalid_argument("Invalid settings UTF-8");
                }
                for (unsigned i = 0; i < count; ++i)
                {
                    if (_position == _text.size() || (uint8_t(_text[_position]) & 0xc0) != 0x80)
                    {
                        throw std::invalid_argument("Invalid settings UTF-8 continuation");
                    }
                    cp = (cp << 6) | (uint8_t(_text[_position++]) & 63);
                }
                if ((count == 1 && cp < 0x80) || (count == 2 && cp < 0x800) || (count == 3 && cp < 0x10000) || cp > 0x10ffff || (cp >= 0xd800 && cp <= 0xdfff))
                {
                    throw std::invalid_argument("Invalid settings UTF-8 scalar");
                }
                utf8(out, cp);
                continue;
            }
            if (_position == _text.size())
            {
                break;
            }
            const char escape = _text[_position++];
            switch (escape)
            {
            case '"': out += '"'; break;
            case '\\': out += '\\'; break;
            case '/': out += '/'; break;
            case 'b': out += '\b'; break;
            case 'f': out += '\f'; break;
            case 'n': out += '\n'; break;
            case 'r': out += '\r'; break;
            case 't': out += '\t'; break;
            case 'u':
            {
                auto cp = hex4();
                if (cp >= 0xd800 && cp <= 0xdbff)
                {
                    if (_position + 2 > _text.size() || _text[_position++] != '\\' || _text[_position++] != 'u')
                    {
                        throw std::invalid_argument("Unpaired Unicode high surrogate");
                    }
                    const auto low = hex4();
                    if (low < 0xdc00 || low > 0xdfff)
                    {
                        throw std::invalid_argument("Invalid Unicode low surrogate");
                    }
                    cp = 0x10000 + ((cp - 0xd800) << 10) + low - 0xdc00;
                }
                else if (cp >= 0xdc00 && cp <= 0xdfff)
                {
                    throw std::invalid_argument("Unpaired Unicode low surrogate");
                }
                utf8(out, cp);
                break;
            }
            default: throw std::invalid_argument("Invalid settings JSON escape");
            }
        }
        throw std::invalid_argument("Unterminated settings JSON string");
    }
    Ptr value(unsigned depth)
    {
        if (depth > 32)
        {
            throw std::invalid_argument("Settings JSON nesting exceeds 32 levels");
        }
        space();
        if (_position == _text.size())
        {
            throw std::invalid_argument("Incomplete settings JSON");
        }
        if (_text[_position] == '"')
        {
            return json::JSON(string());
        }
        if (take('{'))
        {
            json::JsonObject object;
            if (!take('}'))
            {
                do
                {
                    auto key = string();
                    require(':');
                    auto child = value(depth + 1);
                    if (!object.emplace(std::move(key), std::move(child)).second)
                    {
                        throw std::invalid_argument("Duplicate settings JSON key");
                    }
                } while (take(','));
                require('}');
            }
            return json::JSON(std::move(object));
        }
        if (take('['))
        {
            json::JsonList list;
            if (!take(']'))
            {
                do
                {
                    list.push_back(value(depth + 1));
                } while (take(','));
                require(']');
            }
            return json::JSON(std::move(list));
        }
        for (const auto& literal : {std::string("true"), std::string("false"), std::string("null")})
        {
            if (_text.compare(_position, literal.size(), literal) == 0)
            {
                _position += literal.size();
                return literal == "null" ? std::make_shared<Node>() : json::JSON(literal == "true");
            }
        }
        throw std::invalid_argument("Settings values require typed numeric strings, booleans, strings or arrays");
    }
};
std::string quote(const std::string& text)
{
    std::string result = "\"";
    const char* hex = "0123456789abcdef";
    for (const auto c : text)
    {
        if (c == '"' || c == '\\')
        {
            result += '\\';
            result += c;
        }
        else if (uint8_t(c) < 32)
        {
            result += "\\u00";
            result += hex[uint8_t(c) >> 4];
            result += hex[uint8_t(c) & 15];
        }
        else
        {
            result += c;
        }
    }
    result += '"';
    Reader(result).parse(); // Validate raw UTF-8 supplied by programmatic getters too.
    return result;
}
}
const SettingsResult& SettingsImport::result() const
{
    return _result;
}
SettingsResult SettingsImport::apply(SettingsCatalog& catalog) const
{
    auto result = _result;
    if (!result.valid())
    {
        return result;
    }
    if (_generation != catalog.generation())
    {
        result.issues.push_back({"stale_catalog", "", "Settings targets changed after validation"});
        return result;
    }
    auto applied = catalog.apply(_changes);
    result.applied = applied.applied;
    result.issues.insert(result.issues.end(), applied.issues.begin(), applied.issues.end());
    return result;
}
SettingsImport SettingsCodec::prepare(const SettingsCatalog& catalog, std::vector<SettingChange> changes)
{
    SettingsImport plan;
    plan._generation = catalog.generation();
    plan._result = catalog.validate(changes);
    plan._changes = std::move(changes);
    return plan;
}
std::shared_ptr<Node> SettingsCodec::serialize(const SettingsCatalog& catalog)
{
    if (catalog.rendererId().empty())
    {
        throw std::logic_error("Bind settings before serialization");
    }
    json::JsonList components;
    for (const auto& target : catalog._targets)
    {
        json::JsonObject properties;
        for (const auto& p : target.type.properties)
        {
            if (!SettingsCatalog::isGraphicsSetting(catalog.describe(catalog.handle(target.id, p.name))) ||
                p.isReadOnly() || (target.available && !target.available(p.name)))
            {
                continue;
            }
            properties[p.name] = json::JSON(json::JsonObject{
                {"type", json::JSON(typeName(storageType(p)))},
                {"value", encode(p, catalog.read(catalog.handle(target.id, p.name)))}});
        }
        if (properties.empty())
        {
            continue;
        }
        components.push_back(json::JSON(json::JsonObject{{"id", json::JSON(target.id)},
            {"type", json::JSON(target.type.typeName)}, {"properties", json::JSON(std::move(properties))}}));
    }
    return json::JSON(json::JsonObject{{"schema", json::JSON("bg2e.draw.settings")}, {"version", json::JSON("1")},
        {"renderer", json::JSON(catalog.rendererId())}, {"components", json::JSON(std::move(components))}});
}
SettingsImport SettingsCodec::validate(const Node& document, const SettingsCatalog& catalog, UnknownSettingPolicy policy)
{
    SettingsImport plan;
    plan._generation = catalog.generation();
    try
    {
        if (text(field(document, "schema")) != "bg2e.draw.settings" || text(field(document, "version")) != "1" ||
            catalog.rendererId().empty() || text(field(document, "renderer")) != catalog.rendererId())
        {
            throw std::invalid_argument("Incompatible settings schema/version/renderer identity");
        }
        const auto& components = field(document, "components");
        if (!components.isList())
        {
            throw std::invalid_argument("Settings components must be an array");
        }
        std::set<std::string> componentIds;
        for (const auto& component : components.listValue())
        {
            if (!component)
            {
                throw std::invalid_argument("Null settings component");
            }
            const auto id = text(field(*component, "id"));
            const auto type = text(field(*component, "type"));
            if (!componentIds.insert(id).second)
            {
                throw std::invalid_argument("Duplicate settings component identity");
            }
            const auto found = std::find_if(catalog._targets.begin(), catalog._targets.end(), [&](const auto& target)
            {
                return target.id == id;
            });
            const bool known = found != catalog._targets.end();
            if (known && found->type.typeName != type)
            {
                throw std::invalid_argument("Active component type identity differs: " + id);
            }
            const auto& properties = field(*component, "properties");
            if (!properties.isObject())
            {
                throw std::invalid_argument("Settings properties must be an object");
            }
            if (!known)
            {
                plan._result.issues.push_back({"unknown_component", id, "Component is not active", policy == UnknownSettingPolicy::Reject});
            }
            for (const auto& [key, entry] : properties.objectValue())
            {
                if (!entry)
                {
                    throw std::invalid_argument("Null settings property");
                }
                const auto tag = text(field(*entry, "type"));
                auto value = decode(tag, field(*entry, "value")); // Validate syntax even for skipped fields.
                const auto* property = known ? found->type.property(key) : nullptr;
                const bool available = property && (!found->available || found->available(key));
                if (!property || !available)
                {
                    plan._result.issues.push_back({property ? "unavailable_property" : "unknown_property", id + "/" + key,
                        "Setting is not available on the active object", policy == UnknownSettingPolicy::Reject});
                    continue;
                }
                if (!SettingsCatalog::isGraphicsSetting(catalog.describe(catalog.handle(id, key))))
                {
                    plan._result.issues.push_back({"non_graphics_setting", id + "/" + key,
                        "Scene and diagnostic values are never restored as graphics preferences", false});
                    continue;
                }
                if (tag != typeName(storageType(*property)))
                {
                    throw std::invalid_argument("Exact settings property type differs: " + id + "/" + key);
                }
                plan._changes.push_back({catalog.handle(id, key), std::move(value)});
            }
        }
        auto checked = catalog.validate(plan._changes);
        plan._result.issues.insert(plan._result.issues.end(), checked.issues.begin(), checked.issues.end());
    }
    catch (const std::exception& error)
    {
        plan._result.issues.push_back({"invalid_document", "", error.what()});
    }
    return plan;
}
std::shared_ptr<Node> SettingsCodec::parse(const std::string& text)
{
    return Reader(text).parse();
}
std::string SettingsCodec::stringify(const Node& document)
{
    if (document.isString())
    {
        return quote(document.stringValue());
    }
    if (document.isBool())
    {
        return document.boolValue() ? "true" : "false";
    }
    if (document.isNull())
    {
        return "null";
    }
    std::string out;
    if (document.isObject())
    {
        out = "{";
        for (const auto& [key, value] : document.objectValue())
        {
            if (!value)
            {
                throw std::invalid_argument("Null JSON property pointer");
            }
            if (out.size() > 1)
            {
                out += ',';
            }
            out += quote(key) + ':' + stringify(*value);
        }
        return out + '}';
    }
    if (document.isList())
    {
        out = "[";
        for (const auto& value : document.listValue())
        {
            if (!value)
            {
                throw std::invalid_argument("Null JSON array pointer");
            }
            if (out.size() > 1)
            {
                out += ',';
            }
            out += stringify(*value);
        }
        return out + ']';
    }
    throw std::invalid_argument("Untyped JSON numbers are not settings wire values");
}
std::shared_ptr<Node> SettingsCodec::readFile(const std::filesystem::path& path)
{
    std::ifstream stream(path, std::ios::binary);
    if (!stream)
    {
        throw std::runtime_error("Cannot open settings document");
    }
    const std::string text((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
    if (stream.bad())
    {
        throw std::runtime_error("Cannot read settings document");
    }
    return parse(text);
}
void SettingsCodec::exportFile(const SettingsCatalog& catalog, const std::filesystem::path& path, bool includeScene)
{
    auto document = serialize(catalog);
    if (!includeScene)
    {
        auto& components = document->objectValue().at("components")->listValue();
        std::erase_if(components, [](const auto& component)
        {
            const auto id = component->objectValue().at("id")->stringValue();
            return id.starts_with("environment/") || id.starts_with("light/");
        });
    }
    const auto text = stringify(*document);
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    stream.write(text.data(), std::streamsize(text.size()));
    stream.close();
    if (!stream)
    {
        throw std::runtime_error("Cannot write settings document");
    }
}
}
