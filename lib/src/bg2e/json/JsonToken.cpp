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
    while (c == ' ' || c == '\t' || c == '\n')
    {
        stream->get(c);

        if ((c == ' ' || c == '\n' || c == '\t') && !stream->good())
        {
            throw std::logic_error("Ran out of tokens");
        }
        else if (!stream->good())
        {
            return c;
        }
    }
    return c;
}

JsonToken JsonTokenizer::getToken()
{
    char c;
    if (stream->eof())
    {
        throw std::logic_error("Exhaused tokens");
    }
    prevPos = stream->tellg();
    c = getWithoutWhiteSpace();

    struct JsonToken token;
    if (c == '"')
    {
        token.type = JsonTokenType::String;
        token.value = "";
        stream->get(c);
        while (c != '"') {
            token.value += c;
            stream->get(c);
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
        std::streampos prevCharPos = stream->tellg();
        while (c == '-' || (c >= '0' && c <= '9') || c == '.' || c == 'e' || c == '+')
        {
            prevCharPos = stream->tellg();
            stream->get(c);

            if (stream->eof())
            {
                break;
            }
            else
            {
                if (c == '-' || (c >= '0' && c <= '9') || c == '.' || c == 'e' || c == '+')
                {
                    token.value += c;
                }
                else
                {
                    stream->seekg(prevCharPos);
                }
            }
        }
    }
    else if (c == 'f')
    {
        token.type = JsonTokenType::Boolean;
        token.value = "false";
        stream->seekg(4, std::ios_base::cur);
    }
    else if (c == 't')
    {
        token.type = JsonTokenType::Boolean;
        token.value = "true";
        stream->seekg(3, std::ios_base::cur);
    }
    else if (c == 'n')
    {
        token.type = JsonTokenType::NullType;
        stream->seekg(3, std::ios_base::cur);
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
    return token;
}

bool JsonTokenizer::hasMoreTokens()
{
    size_t prevPos = stream->tellg();
    bool result = true;
    try
    {
        getToken();
    }
    catch(std::exception &)
    {
        result = false;
    }
    stream->seekg(prevPos);
    return result;
}

void JsonTokenizer::rollBackToken()
{
    if (stream->eof())
    {
        stream->clear();
    }
    stream->seekg(prevPos);
}


}
