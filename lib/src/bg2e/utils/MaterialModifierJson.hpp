#pragma once

#include <bg2e/json/JsonNode.hpp>

#include <charconv>
#include <cmath>
#include <stdexcept>
#include <string_view>

namespace bg2e::utils::detail {

// Material input must fail as a whole on malformed JSON. The general engine
// parser currently tolerates missing separators and can return partial roots.
// Keep this strict reader private to the modifier, producing native JsonNodes.
class MaterialModifierJson {
public:
    explicit MaterialModifierJson(std::string_view text) : _text(text) {}

    std::shared_ptr<json::JsonNode> parse()
    {
        auto result = value(0);
        space();
        if (_pos != _text.size()) fail();
        return result;
    }

private:
    std::string_view _text;
    std::size_t _pos = 0;

    [[noreturn]] static void fail() { throw std::invalid_argument("Invalid material JSON"); }
    char peek() const { return _pos < _text.size() ? _text[_pos] : '\0'; }
    void space()
    {
        while (peek() == ' ' || peek() == '\t' || peek() == '\r' || peek() == '\n') ++_pos;
    }
    bool consume(char c)
    {
        space();
        if (peek() != c || _pos == _text.size()) return false;
        ++_pos;
        return true;
    }
    void expect(char c) { if (!consume(c)) fail(); }
    static bool digit(char c) { return c >= '0' && c <= '9'; }

    uint32_t hex()
    {
        uint32_t result = 0;
        for (int i = 0; i < 4; ++i)
        {
            if (_pos == _text.size()) fail();
            char c = _text[_pos++];
            result <<= 4;
            if (digit(c)) result += c - '0';
            else if (c >= 'a' && c <= 'f') result += c - 'a' + 10;
            else if (c >= 'A' && c <= 'F') result += c - 'A' + 10;
            else fail();
        }
        return result;
    }

    static void utf8(std::string& out, uint32_t code)
    {
        if (code <= 0x7f) out += static_cast<char>(code);
        else
        {
            if (code > 0xffff) out += static_cast<char>(0xf0 | (code >> 18));
            if (code > 0x7ff) out += static_cast<char>((code > 0xffff ? 0x80 : 0xe0) | ((code >> 12) & 0x3f));
            out += static_cast<char>((code > 0x7ff ? 0x80 : 0xc0) | ((code >> 6) & 0x3f));
            out += static_cast<char>(0x80 | (code & 0x3f));
        }
    }

    std::string string()
    {
        expect('"');
        std::string result;
        while (_pos < _text.size())
        {
            unsigned char c = _text[_pos++];
            if (c == '"') return result;
            if (c < 0x20) fail();
            if (c != '\\') { result += static_cast<char>(c); continue; }
            if (_pos == _text.size()) fail();
            switch (_text[_pos++])
            {
            case '"': result += '"'; break;
            case '\\': result += '\\'; break;
            case '/': result += '/'; break;
            case 'b': result += '\b'; break;
            case 'f': result += '\f'; break;
            case 'n': result += '\n'; break;
            case 'r': result += '\r'; break;
            case 't': result += '\t'; break;
            case 'u':
            {
                auto code = hex();
                if (code >= 0xd800 && code <= 0xdbff)
                {
                    if (_text.substr(_pos, 2) != "\\u") fail();
                    _pos += 2;
                    auto low = hex();
                    if (low < 0xdc00 || low > 0xdfff) fail();
                    code = 0x10000 + ((code - 0xd800) << 10) + low - 0xdc00;
                }
                else if (code >= 0xdc00 && code <= 0xdfff) fail();
                utf8(result, code);
                break;
            }
            default: fail();
            }
        }
        fail();
    }

    std::shared_ptr<json::JsonNode> value(unsigned depth)
    {
        if (depth > 128) fail();
        space();
        if (peek() == '"') return json::JSON(string());
        if (consume('{'))
        {
            json::JsonObject object;
            if (consume('}')) return json::JSON(object);
            do
            {
                auto key = string();
                expect(':');
                object[key] = value(depth + 1);
                if (consume('}')) return json::JSON(object);
                expect(',');
            } while (true);
        }
        if (consume('['))
        {
            json::JsonList list;
            if (consume(']')) return json::JSON(list);
            do
            {
                list.push_back(value(depth + 1));
                if (consume(']')) return json::JSON(list);
                expect(',');
            } while (true);
        }
        for (auto literal : {"true", "false", "null"})
        {
            std::string_view word(literal);
            if (_text.substr(_pos, word.size()) == word)
            {
                _pos += word.size();
                if (word == "null") return std::make_shared<json::JsonNode>();
                return json::JSON(word == "true");
            }
        }
        auto start = _pos;
        if (peek() == '-') ++_pos;
        if (peek() == '0') ++_pos;
        else
        {
            if (!digit(peek())) fail();
            while (digit(peek())) ++_pos;
        }
        if (peek() == '.')
        {
            ++_pos;
            if (!digit(peek())) fail();
            while (digit(peek())) ++_pos;
        }
        if (peek() == 'e' || peek() == 'E')
        {
            ++_pos;
            if (peek() == '+' || peek() == '-') ++_pos;
            if (!digit(peek())) fail();
            while (digit(peek())) ++_pos;
        }
        float number = 0;
        auto [end, error] = std::from_chars(_text.data() + start, _text.data() + _pos, number);
        if (error != std::errc{} || end != _text.data() + _pos || !std::isfinite(number)) fail();
        return json::JSON(number);
    }
};

}
