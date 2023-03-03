
#include <bg2e/tools/Json.hpp>

#include <sstream>
#include <iostream>

namespace bg2e {
namespace tools {

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

JsonNode::JsonNode(int32_t p) {
    setValue(p);
}

JsonNode::JsonNode(uint32_t p) {
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

JsonNode::JsonNode(const glm::vec2& p) {
    setValue(p);
}

JsonNode::JsonNode(const glm::vec3& p) {
    setValue(p);
}

JsonNode::JsonNode(const glm::vec4& p) {
    setValue(p);
}

JsonNode::JsonNode(const glm::mat4& p) {
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
        outputString += std::to_string(_numberValue);
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
            outputString += innerIndentation + node->toString(indentationLevel + 1);
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

std::shared_ptr<JsonNode> JSON(int32_t p)
{
    return std::make_shared<JsonNode>(p);
}

std::shared_ptr<JsonNode> JSON(uint32_t p)
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

std::shared_ptr<JsonNode> JSON(const glm::vec2& v)
{
    return std::make_shared<JsonNode>(v);
}

std::shared_ptr<JsonNode> JSON(const glm::vec3& v)
{
    return std::make_shared<JsonNode>(v);
}

std::shared_ptr<JsonNode> JSON(const glm::vec4& v)
{
    return std::make_shared<JsonNode>(v);
}

std::shared_ptr<JsonNode> JSON(const glm::mat4& v)
{
    return std::make_shared<JsonNode>(v);
}

}
}
