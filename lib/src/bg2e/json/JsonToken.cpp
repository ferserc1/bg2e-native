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

#include <bg2e/json/JsonToken.hpp>

namespace bg2e::json {

std::string JsonToken::toString()
{
    switch (type)
    {
    case JsonTokenType::CurlyOpen:
        return "Curly open";
    case JsonTokenType::CurlyClose:
        return "Curly close";
    case JsonTokenType::Colon:
        return "COLON";
    case JsonTokenType::Number:
        return "Number: " + value;
    case JsonTokenType::String:
        return "String: " + value;
    case JsonTokenType::ListOpen:
        return "Array open";
    case JsonTokenType::ListClose:
        return "Array close";
    case JsonTokenType::Comma:
        return "Comma";
    case JsonTokenType::Boolean:
        return "Boolean: " + value;
        ;
    case JsonTokenType::NullType:
        return "Null";
    default:
        return "";
    }
}

JsonTokenizer::JsonTokenizer(std::istream* s)
    : stream(s)
{
}

char JsonTokenizer::getWithoutWhiteSpace()
{
    char c = ' ';
    while (c == ' ' || c == '\t' || c == '\n' || c == '\r')
    {
        if (!stream->get(c))
        {
            throw std::logic_error("Ran out of tokens");
        }
    }
    return c;
}

JsonToken JsonTokenizer::getToken()
{
    if (replayToken)
    {
        replayToken = false;
        return lastToken;
    }
    char c = getWithoutWhiteSpace();

    struct JsonToken token;
    if (c == '"')
    {
        token.type = JsonTokenType::String;
        token.value = "";
        auto hex4 = [&]()
        {
            uint32_t value = 0;
            for (int i = 0; i < 4; ++i)
            {
                if (!stream->get(c))
                {
                    throw std::logic_error("Truncated JSON Unicode escape");
                }
                const int digit = c >= '0' && c <= '9' ? c - '0' :
                    c >= 'a' && c <= 'f' ? c - 'a' + 10 : c >= 'A' && c <= 'F' ? c - 'A' + 10 : -1;
                if (digit < 0)
                {
                    throw std::logic_error("Invalid JSON Unicode escape");
                }
                value = (value << 4) | static_cast<uint32_t>(digit);
            }
            return value;
        };
        bool closed = false;
        while (stream->get(c))
        {
            if (c == '"')
            {
                closed = true;
                break;
            }
            if (static_cast<unsigned char>(c) < 0x20)
            {
                throw std::logic_error("Unescaped JSON control character");
            }
            if (c != '\\')
            {
                token.value += c;
                continue;
            }
            if (!stream->get(c))
            {
                throw std::logic_error("Truncated JSON escape");
            }
            switch (c)
            {
            case '"': case '\\': case '/': token.value += c; break;
            case 'b': token.value += '\b'; break;
            case 'f': token.value += '\f'; break;
            case 'n': token.value += '\n'; break;
            case 'r': token.value += '\r'; break;
            case 't': token.value += '\t'; break;
            case 'u':
            {
                uint32_t code = hex4();
                if (code >= 0xD800 && code <= 0xDBFF)
                {
                    if (!stream->get(c) || c != '\\' || !stream->get(c) || c != 'u')
                    {
                        throw std::logic_error("Missing JSON low surrogate");
                    }
                    const auto low = hex4();
                    if (low < 0xDC00 || low > 0xDFFF)
                    {
                        throw std::logic_error("Invalid JSON low surrogate");
                    }
                    code = 0x10000 + ((code - 0xD800) << 10) + low - 0xDC00;
                }
                else if (code >= 0xDC00 && code <= 0xDFFF)
                {
                    throw std::logic_error("Unpaired JSON low surrogate");
                }
                if (code < 0x80)
                {
                    token.value += static_cast<char>(code);
                }
                else
                {
                    if (code >= 0x10000)
                    {
                        token.value += static_cast<char>(0xF0 | (code >> 18));
                        token.value += static_cast<char>(0x80 | ((code >> 12) & 63));
                    }
                    else if (code >= 0x800)
                    {
                        token.value += static_cast<char>(0xE0 | (code >> 12));
                    }
                    else
                    {
                        token.value += static_cast<char>(0xC0 | (code >> 6));
                    }
                    if (code >= 0x800)
                    {
                        token.value += static_cast<char>(0x80 | ((code >> 6) & 63));
                    }
                    token.value += static_cast<char>(0x80 | (code & 63));
                }
                break;
            }
            default: throw std::logic_error("Invalid JSON escape");
            }
        }
        if (!closed)
        {
            throw std::logic_error("Unterminated JSON string");
        }
    }
    else if (c == '{')
    {
        token.type = JsonTokenType::CurlyOpen;
    }
    else if (c == '}')
    {
        token.type = JsonTokenType::CurlyClose;
    }
    else if (c == '-' || (c >= '0' && c <= '9') || c == '.' || c == 'e' || c == '+')
    {
        token.type = JsonTokenType::Number;
        token.value = "";
        token.value += c;
        // Leave the delimiter (including either newline character) unread.
        // Text-mode file positions are not portable byte offsets.
        while (stream->good())
        {
            auto next = stream->peek();
            if (!(next == '-' || (next >= '0' && next <= '9') ||
                  next == '.' || next == 'e' || next == '+'))
            {
                break;
            }
            if (!stream->get(c))
            {
                throw std::logic_error("Ran out of tokens");
            }
            token.value += c;
        }
    }
    else if (c == 'f')
    {
        token.type = JsonTokenType::Boolean;
        token.value = "false";
        stream->ignore(4);
    }
    else if (c == 't')
    {
        token.type = JsonTokenType::Boolean;
        token.value = "true";
        stream->ignore(3);
    }
    else if (c == 'n')
    {
        token.type = JsonTokenType::NullType;
        stream->ignore(3);
    }
    else if (c == '[')
    {
        token.type = JsonTokenType::ListOpen;
    }
    else if (c == ']')
    {
        token.type = JsonTokenType::ListClose;
    }
    else if (c == ':')
    {
        token.type = JsonTokenType::Colon;
    }
    else if (c == ',')
    {
        token.type = JsonTokenType::Comma;
    }
    lastToken = token;
    hasLastToken = true;
    return token;
}

bool JsonTokenizer::hasMoreTokens()
{
    try
    {
        getToken();
        rollBackToken();
        return true;
    }
    catch (std::exception&)
    {
        return false;
    }
}

void JsonTokenizer::rollBackToken()
{
    replayToken = hasLastToken;
}

}
