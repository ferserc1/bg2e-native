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

#include <bg2e/json/JsonParser.hpp>

#include <limits>
#include <charconv>
#include <string_view>
#include <system_error>

namespace bg2e::json {

std::shared_ptr<JsonNode> & JsonParser::parse()
{
    try
    {
        // A JSON document contains exactly one root value. Parse it from the
        // first token instead of looping over every token in the stream.
        if (tokenizer.hasMoreTokens())
        {
            JsonToken token = tokenizer.getToken();
            switch (token.type) {
            case JsonTokenType::CurlyOpen:
                root = parseObject();
                break;
            case JsonTokenType::ListOpen:
                root = parseList();
                break;
            case JsonTokenType::String:
                tokenizer.rollBackToken();
                root = parseString();
                break;
            case JsonTokenType::Number:
                tokenizer.rollBackToken();
                root = parseNumber();
                break;
            case JsonTokenType::Boolean:
                tokenizer.rollBackToken();
                root = parseBoolean();
                break;
            case JsonTokenType::NullType:
                root = parseNull();
                break;
            default:
                throw std::logic_error("Unexpected token at the start of the JSON document");
            }
        }

        // Once the root value has been parsed, the end of the file must have
        // been reached. Any remaining token means the input is malformed (for
        // example, extra bytes after the closing bracket caused by an over-read
        // of the source buffer). Detect it here and fail, instead of looping
        // over the trailing tokens indefinitely.
        if (tokenizer.hasMoreTokens())
        {
            throw std::logic_error("Expected end of file but found additional tokens after the root JSON value");
        }
    }
    catch(std::logic_error &e)
    {
        std::cout << "Warning:" << e.what() << std::endl;
    }
    return root;
}

std::shared_ptr<JsonNode> JsonParser::parseObject()
{
    std::shared_ptr<JsonNode> node = std::make_shared<JsonNode>();
    JsonObject keyObjectMap;
    bool hasCompleted = false;
    while (!hasCompleted)
    {
        if (tokenizer.hasMoreTokens())
        {
            JsonToken nextToken = tokenizer.getToken();
            if (nextToken.type == JsonTokenType::CurlyClose)
            {
                // Empty object
                hasCompleted = true;
            }
            else
            {
                std::string key = nextToken.value;
                tokenizer.getToken();
                nextToken = tokenizer.getToken();
                std::shared_ptr<JsonNode> node;
                switch (nextToken.type) {
                case JsonTokenType::String:
                    tokenizer.rollBackToken();
                    keyObjectMap[key] = parseString();
                    break;
                case JsonTokenType::ListOpen:
                    keyObjectMap[key] = parseList();
                    break;
                case JsonTokenType::Number:
                    tokenizer.rollBackToken();
                    keyObjectMap[key] = parseNumber();
                    break;
                case JsonTokenType::CurlyOpen:
                    keyObjectMap[key] = parseObject();
                    break;
                case JsonTokenType::Boolean:
                    tokenizer.rollBackToken();
                    keyObjectMap[key] = parseBoolean();
                    break;
                case JsonTokenType::NullType:
                    keyObjectMap[key] = parseNull();
                    break;
                default:
                    break;
                }

                nextToken = tokenizer.getToken();
                if (nextToken.type == JsonTokenType::CurlyClose)
                {
                    hasCompleted = true;
                    break;
                }
                else if (nextToken.type == JsonTokenType::Comma)
                {
                    // Nothing to do
                }
            }
        }
        else
        {
                throw std::logic_error("No more tokens");
        }
    }
    node->setValue(keyObjectMap);
    return node;
}

std::shared_ptr<JsonNode> JsonParser::parseString()
{
    std::shared_ptr<JsonNode> node = std::make_shared<JsonNode>();
    JsonToken token = tokenizer.getToken();
    std::string sValue(token.value);
    node->setValue(sValue);
    return node;
}

std::shared_ptr<JsonNode> JsonParser::parseNumber()
{
    std::shared_ptr<JsonNode> node = std::make_shared<JsonNode>();
    JsonToken nextToken = tokenizer.getToken();
    std::string value = nextToken.value;
    
    #ifdef BG2E_LINUX
    // Linux version: based in from_chars because stof and stod depends on system localization
    // settings, and will don't work in some locations where the comma is the decimal separator
    double dValue = 0.0;
    auto [ptr, ec] = std::from_chars(
        value.data(),
        value.data() + value.size(),
        dValue
    );

    if (ec == std::errc::invalid_argument)
    {
        std::cerr << "WARN: Invalid argument parsing JSON number" << std::endl;
        node->setValue(0);
    }
    else if (ec == std::errc::result_out_of_range ||
            static_cast<float>(dValue) == std::numeric_limits<float>::infinity()
    ) {
        node->setValue(std::numeric_limits<float>::max());
    }
    else {
        node->setValue(static_cast<float>(dValue));
    }
    
    #else
    // Mac/Windows version: the stod version is tested and it works well. Apart from that, the
    // from_chars method does not work in macOS until version 26
    try
    {
        float fValue = stof(value);
        node->setValue(fValue);
    }
    catch (std::out_of_range)
    {
        double dValue = stod(value);
        if (static_cast<float>(dValue) == std::numeric_limits<float>::infinity())
        {
            node->setValue(std::numeric_limits<float>::max());
        }
        else if (static_cast<float>(dValue) == -std::numeric_limits<float>::infinity())
        {
            node->setValue(std::numeric_limits<float>::min());
        }
    }
    #endif
    
    return node;
}

std::shared_ptr<JsonNode> JsonParser::parseList()
{
    std::shared_ptr<JsonNode> node = std::make_shared<JsonNode>();
    JsonList list;
    bool hasCompleted = false;
    std::shared_ptr<JsonNode> childNode;
    while (!hasCompleted)
    {
        if (!tokenizer.hasMoreTokens())
        {
            hasCompleted = true;
        }
        else
        {
            JsonToken nextToken = tokenizer.getToken();
            switch (nextToken.type) {
            case JsonTokenType::ListOpen:
                childNode = parseList();
                break;
            case JsonTokenType::CurlyOpen:
                childNode = parseObject();
                break;
            case JsonTokenType::String:
                tokenizer.rollBackToken();
                childNode = parseString();
                break;
            case JsonTokenType::Number:
                tokenizer.rollBackToken();
                childNode = parseNumber();
                break;
            case JsonTokenType::Boolean:
                tokenizer.rollBackToken();
                childNode = parseBoolean();
                break;
            case JsonTokenType::NullType:
                childNode = parseNull();
                break;
            case JsonTokenType::Comma:
                if (childNode != nullptr) {
                    list.push_back(childNode);
                }
                else {
                    throw std::logic_error("Unexpected null node found in list");
                }
                break;
            case JsonTokenType::ListClose:
                if (childNode != nullptr) {
                    list.push_back(childNode);
                }
                hasCompleted = true;
                break;
            default:
                break;
            }
        }
    }
    node->setValue(list);
    return node;
}

std::shared_ptr<JsonNode> JsonParser::parseBoolean()
{
    std::shared_ptr<JsonNode> node = std::make_shared<JsonNode>();
    JsonToken nextToken = tokenizer.getToken();
    node->setValue(nextToken.value == "true" ? true : false);
    return node;
}

std::shared_ptr<JsonNode> JsonParser::parseNull()
{
    std::shared_ptr<JsonNode> node = std::make_shared<JsonNode>();
    node->setNull();
    return node;
}

}
