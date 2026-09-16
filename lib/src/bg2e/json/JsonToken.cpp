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
        return "Boolean: " + value;;
    case JsonTokenType::NullType:
        return "Null";
    default:
        return "";
    }
}

JsonTokenizer::JsonTokenizer(std::istream * s)
    :stream(s)
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
        while (stream->get(c) && c != '"') {
            token.value += c;
        }
        if (c != '"') {
            // Reached the end of the stream without a closing quote. Without
            // this guard the loop would spin forever (a failed get() leaves
            // 'c' unchanged), which is exactly what happens when a corrupt or
            // over-read buffer feeds an unterminated string to the tokenizer.
            throw std::logic_error("Unterminated string while reading JSON token");
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
    else if (c == '-' || (c>='0' && c <='9') || c == '.' || c == 'e' || c == '+')
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
    catch(std::exception &)
    {
        return false;
    }
}

void JsonTokenizer::rollBackToken()
{
    replayToken = hasLastToken;
}


}
