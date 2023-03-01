
#include <bg2e/tools/JsonParser.hpp>

namespace bg2e {
namespace tools {

std::shared_ptr<JsonNode> & JsonParser::parse() {
    std::string key = "";
    while (tokenizer.hasMoreTokens()) {
        JsonToken token;
        try {
            token = tokenizer.getToken();
            switch (token.type) {
            case JsonTokenType::CurlyOpen: {
                std::shared_ptr<JsonNode> parsedObject = parseObject();
                if (!root) {
                    root = parsedObject;
                }
                break;
            }
            case JsonTokenType::ListOpen: {
                std::shared_ptr<JsonNode> parsedList = parseList();
                if (!root) {
                    root = parsedList;
                }
                break;
            }
            case JsonTokenType::String: {
                tokenizer.rollBackToken();
                std::shared_ptr<JsonNode> parsedString = parseString();
                if (!root) {
                    root = parsedString;
                }
                break;
            }
            case JsonTokenType::Number: {
                tokenizer.rollBackToken();
                std::shared_ptr<JsonNode> parsedNumber = parseNumber();
                if (!root) {
                    root = parsedNumber;
                }
            }
            case JsonTokenType::Boolean: {
                tokenizer.rollBackToken();
                std::shared_ptr<JsonNode> parsedBoolean = parseBoolean();
                break;
            }
            default:
                break;
            }
        }
        catch(std::logic_error &e) {
            std::cout << "Warning:" << e.what() << std::endl;
            break;
        }
    }
    return root;
}

std::shared_ptr<JsonNode> JsonParser::parseObject() {
    std::shared_ptr<JsonNode> node = std::make_shared<JsonNode>();
    JsonObject keyObjectMap;
    bool hasCompleted = false;
    while (!hasCompleted) {
        if (tokenizer.hasMoreTokens()) {
            JsonToken nextToken = tokenizer.getToken();
            if (nextToken.type == JsonTokenType::CurlyClose) {
                // Empty object
                hasCompleted = true;
            }
            else {
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
                if (nextToken.type == JsonTokenType::CurlyClose) {
                    hasCompleted = true;
                    break;
                }
                else if (nextToken.type == JsonTokenType::Comma) {
                    // Nothing to do
                }
            }
        }
        else {
                throw std::logic_error("No more tokens");
        }
    }
    node->setValue(keyObjectMap);
    return node;
}

std::shared_ptr<JsonNode> JsonParser::parseString() {
    std::shared_ptr<JsonNode> node = std::make_shared<JsonNode>();
    JsonToken token = tokenizer.getToken();
    std::string sValue(token.value);
    node->setValue(sValue);
    return node;
}

std::shared_ptr<JsonNode> JsonParser::parseNumber() {
    std::shared_ptr<JsonNode> node = std::make_shared<JsonNode>();
    JsonToken nextToken = tokenizer.getToken();
    std::string value = nextToken.value;
    float fValue = std::stof(value);
    node->setValue(fValue);
    return node;
}

std::shared_ptr<JsonNode> JsonParser::parseList() {
    std::shared_ptr<JsonNode> node = std::make_shared<JsonNode>();
    JsonList list;
    bool hasCompleted = false;
    std::shared_ptr<JsonNode> childNode;
    while (!hasCompleted) {
        if (!tokenizer.hasMoreTokens()) {
            hasCompleted = true;
        }
        else {
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

std::shared_ptr<JsonNode> JsonParser::parseBoolean() {
    std::shared_ptr<JsonNode> node = std::make_shared<JsonNode>();
    JsonToken nextToken = tokenizer.getToken();
    node->setValue(nextToken.value == "true" ? true : false);
    return node;
}

std::shared_ptr<JsonNode> JsonParser::parseNull() {
    std::shared_ptr<JsonNode> node = std::make_shared<JsonNode>();
    node->setNull();
    return node;
}

}
}
