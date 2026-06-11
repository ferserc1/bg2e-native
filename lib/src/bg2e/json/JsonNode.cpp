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

#include <format>
#include <bg2e/json/JsonNode.hpp>

#include <iostream>

namespace bg2e::json {

JsonNode JsonNode::s_nullValue;

JsonNode::JsonNode() :type(Type::Null)
{
    
}

JsonNode::JsonNode(Type t) :type(t)
{

}

JsonNode::JsonNode(const JsonObject& p) {
    setValue(p);
}

JsonNode::JsonNode(JsonObject&& p) {
    setValue(p);
}

JsonNode::JsonNode(const JsonList& p) {
    setValue(p);
}

JsonNode::JsonNode(JsonList&& p) {
    setValue(p);
}

JsonNode::JsonNode(const char* p) {
    setValue(p);
}

JsonNode::JsonNode(std::string&& p) {
    setValue(p);
}

JsonNode::JsonNode(const std::string & p) {
    setValue(p);
}

JsonNode::JsonNode(char p) {
    setValue(p);
}

JsonNode::JsonNode(int8_t p) {
    setValue(p);
}

JsonNode::JsonNode(int16_t p) {
    setValue(p);
}

JsonNode::JsonNode(int32_t p) {
    setValue(p);
}

JsonNode::JsonNode(int64_t p) {
    setValue(p);
}

JsonNode::JsonNode(uint8_t p) {
    setValue(p);
}

JsonNode::JsonNode(uint16_t p) {
    setValue(p);
}

JsonNode::JsonNode(uint32_t p) {
    setValue(p);
}

JsonNode::JsonNode(uint64_t p) {
    setValue(p);
}

JsonNode::JsonNode(float p) {
    setValue(p);
}

JsonNode::JsonNode(double p) {
    setValue(p);
}

JsonNode::JsonNode(bool p) {
    setValue(p);
}

JsonNode::JsonNode(const base::Color & p)
{
    setValue(p);
}

JsonNode::JsonNode(const glm::vec2 & p) {
    setValue(p);
}

JsonNode::JsonNode(const glm::vec3 & p) {
    setValue(p);
}

JsonNode::JsonNode(const glm::vec4 & p) {
    setValue(p);
}

JsonNode::JsonNode(const glm::mat3 & p) {
    setValue(p);
}

JsonNode::JsonNode(const glm::mat4 & p) {
    setValue(p);
}

JsonNode::JsonNode(const std::array<float, 2> & p) {
    setValue(p);
}

JsonNode::JsonNode(const std::array<float, 3> & p) {
    setValue(p);
}

JsonNode::JsonNode(const std::array<float, 4> & p) {
    setValue(p);
}

JsonNode::JsonNode(const std::array<float, 9> & p) {
    setValue(p);
}

JsonNode::JsonNode(const std::array<float, 16> & p) {
    setValue(p);
}


JsonNode::~JsonNode() {
}

void JsonNode::printNode(int indentationLevel) {
    std::cout << toString(indentationLevel);
}

std::string JsonNode::toString(int indentationLevel) {
    std::string indentation = std::string(indentationLevel * 2, ' ');
    std::string innerIndentation = std::string((indentationLevel + 1) * 2, ' ');
    std::string outputString = "";

    switch (type) {
    case Type::String:
        outputString += "\"" + _stringValue + "\"";
        break;
    case Type::Number:
        outputString += std::format("{}", _numberValue);
        break;
    case Type::Bool:
        outputString += (_boolValue ? "true" : "false");
        break;
    case Type::Null:
        outputString += "null";
        break;
    
    case Type::List: {
        outputString += "[";
        size_t index = 0;
        bool emptyList = true;
        for (auto node : (_listValue)) {
            if (emptyList) {
                outputString += "\n";
            }
            emptyList = false;
            if (!node)
            {
                outputString += innerIndentation + "null";
            }
            else
            {
                outputString += innerIndentation + node->toString(indentationLevel + 1);
            }
            if (index < (_listValue).size() - 1) {
                outputString += ",";
            }
            outputString += "\n";
            index++;
        }
        outputString += (emptyList ? "" : indentation) + "]";
        break;
    }
    case Type::Object: {
        outputString += "{";
        bool emptyObject = true;
        for (JsonObject::iterator i = (_objectValue).begin();
            i != (_objectValue).end(); ++i) {
            if (emptyObject) {
                outputString += "\n";
            }
            emptyObject = false;
            outputString += innerIndentation + "\"" + i->first + "\" : ";
            outputString += i->second->toString(indentationLevel + 1);
            JsonObject::iterator next = i;
            next++;
            if (next != (_objectValue).end()) {
                outputString += ",";
            }
            outputString += "\n";
        }
        outputString += (emptyObject ? "" : indentation) + "}";
        break;
    }
    }
    return outputString;
}

std::string JsonNode::serialize()
{
    std::setlocale(LC_NUMERIC, "C");
    std::string outputString = "";

    switch (type) {
    case Type::String:
        outputString += "\"" + _stringValue + "\"";
        break;
    case Type::Number:
        outputString += std::format("{}", _numberValue);
        break;
    case Type::Bool:
        outputString += (_boolValue ? "true" : "false");
        break;
    case Type::Null:
        outputString += "null";
        break;
    
    case Type::List: {
        outputString += "[";
        size_t index = 0;
        for (auto node : (_listValue)) {
            outputString += node->serialize();
            if (index < (_listValue).size() - 1) {
                outputString += ",";
            }
            index++;
        }
        outputString += "]";
        break;
    }
    case Type::Object: {
        outputString += "{";
        for (JsonObject::iterator i = (_objectValue).begin();
            i != (_objectValue).end(); ++i) {
            outputString += "\"" + i->first + "\":";
            outputString += i->second->serialize();
            JsonObject::iterator next = i;
            next++;
            if (next != (_objectValue).end()) {
                outputString += ",";
            }
        }
        outputString += "}";
        break;
    }
    }
    return outputString;
}

std::shared_ptr<JsonNode> JSON(const JsonObject& p)
{
    return std::make_shared<JsonNode>(p);
}

std::shared_ptr<JsonNode> JSON(JsonObject&& p)
{
    return std::make_shared<JsonNode>(p);
}

std::shared_ptr<JsonNode> JSON(const JsonList& p)
{
    return std::make_shared<JsonNode>(p);
}

std::shared_ptr<JsonNode> JSON(JsonList&& p)
{
    return std::make_shared<JsonNode>(p);
}

std::shared_ptr<JsonNode> JSON(const char* p)
{
    return std::make_shared<JsonNode>(p);
}

std::shared_ptr<JsonNode> JSON(std::string&& p)
{
    return std::make_shared<JsonNode>(p);
}

std::shared_ptr<JsonNode> JSON(const std::string& p)
{
    return std::make_shared<JsonNode>(p);
}

std::shared_ptr<JsonNode> JSON(char p)
{
    return std::make_shared<JsonNode>(p);
}

std::shared_ptr<JsonNode> JSON(int8_t p)
{
    return std::make_shared<JsonNode>(p);
}

std::shared_ptr<JsonNode> JSON(int16_t p)
{
    return std::make_shared<JsonNode>(p);
}

std::shared_ptr<JsonNode> JSON(int32_t p)
{
    return std::make_shared<JsonNode>(p);
}

std::shared_ptr<JsonNode> JSON(int64_t p)
{
    return std::make_shared<JsonNode>(p);
}

std::shared_ptr<JsonNode> JSON(uint8_t p)
{
    return std::make_shared<JsonNode>(p);
}

std::shared_ptr<JsonNode> JSON(uint16_t p)
{
    return std::make_shared<JsonNode>(p);
}

std::shared_ptr<JsonNode> JSON(uint32_t p)
{
    return std::make_shared<JsonNode>(p);
}

std::shared_ptr<JsonNode> JSON(uint64_t p)
{
    return std::make_shared<JsonNode>(p);
}

std::shared_ptr<JsonNode> JSON(float p)
{
    return std::make_shared<JsonNode>(p);
}

std::shared_ptr<JsonNode> JSON(double p)
{
    return std::make_shared<JsonNode>(p);
}

std::shared_ptr<JsonNode> JSON(bool p)
{
    return std::make_shared<JsonNode>(p);
}

std::shared_ptr<JsonNode> JSON(const base::Color & p)
{
    return std::make_shared<JsonNode>(p);
}

std::shared_ptr<JsonNode> JSON(const glm::vec2 & p)
{
    return std::make_shared<JsonNode>(p);
}

std::shared_ptr<JsonNode> JSON(const glm::vec3 & p)
{
    return std::make_shared<JsonNode>(p);
}

std::shared_ptr<JsonNode> JSON(const glm::vec4 & p)
{
    return std::make_shared<JsonNode>(p);
}

std::shared_ptr<JsonNode> JSON(const glm::mat3 & p)
{
    return std::make_shared<JsonNode>(p);
}

std::shared_ptr<JsonNode> JSON(const glm::mat4 & p)
{
    return std::make_shared<JsonNode>(p);
}

std::shared_ptr<JsonNode> JSON(const std::array<float, 2> & p)
{
    return std::make_shared<JsonNode>(p);
}

std::shared_ptr<JsonNode> JSON(const std::array<float, 3> & p)
{
    return std::make_shared<JsonNode>(p);
}

std::shared_ptr<JsonNode> JSON(const std::array<float, 4> & p)
{
    return std::make_shared<JsonNode>(p);
}

std::shared_ptr<JsonNode> JSON(const std::array<float, 9> & p)
{
    return std::make_shared<JsonNode>(p);
}

std::shared_ptr<JsonNode> JSON(const std::array<float, 16> & p)
{
    return std::make_shared<JsonNode>(p);
}


}
