#ifndef bg2e_tools_json_hpp
#define bg2e_tools_json_hpp

#include <bg2e/export.hpp>

#include <string>
#include <map>
#include <vector>
#include <exception>
#include <stdexcept>
#include <memory>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

namespace bg2e {
namespace tools {

class JsonNode;

using JsonObject = std::map<std::string, std::shared_ptr<JsonNode>>;

using JsonList = std::vector<std::shared_ptr<JsonNode>>;

class BG2E_EXPORT JsonNode {
    enum class Type {
        Object,
        List,
        String,
        Number,
        Bool,
        Null
    };

    JsonObject _objectValue = {};
    JsonList _listValue = {};
    std::string _stringValue = "";
    float _numberValue = 0.0f;
    bool _boolValue = false;

    Type type;

public:
    JsonNode();
    JsonNode(Type t);
    JsonNode(const JsonObject&);
    JsonNode(JsonObject&&);
    JsonNode(const JsonList&);
    JsonNode(JsonList&&);
    JsonNode(const char*);
    JsonNode(std::string&&);
    JsonNode(const std::string &);
    JsonNode(char);
    JsonNode(int32_t);
    JsonNode(uint32_t);
    JsonNode(float);
    JsonNode(double);
    JsonNode(bool);
    JsonNode(const glm::vec2&);
    JsonNode(const glm::vec3&);
    JsonNode(const glm::vec4&);
    JsonNode(const glm::mat4&);
    virtual ~JsonNode();
    
    JsonObject& objectValue() {
        if (type == Type::Object) {
            return _objectValue;
        }
        throw std::logic_error("Improper return type: object");
    }

    const JsonObject& objectValue() const {
        if (type == Type::Object) {
            return _objectValue;
        }
        throw std::logic_error("Improper return type: object");
    }
    
    JsonObject& objectValue(JsonObject&& defaultValue) {
        if (type == Type::Object) {
            return _objectValue;
        }
        else {
            return defaultValue;
        }
    }
    
    const JsonObject& objectValue(JsonObject&& defaultValue) const {
        if (type == Type::Object) {
            return _objectValue;
        }
        else {
            return defaultValue;
        }
    }
    
    std::shared_ptr<JsonNode> operator[](const std::string& key) {
        if (type == Type::Object) {
            return _objectValue[key];
        }
        throw std::logic_error("Improper return type: object");
    }

    JsonList& listValue() {
        if (type == Type::List) {
            return _listValue;
        }
        throw std::logic_error("Improper return type: object");
    }

    const JsonList& listValue() const {
        if (type == Type::List) {
            return _listValue;
        }
        throw std::logic_error("Improper return type: object");
    }
    
    JsonList& listValue(JsonList&& defaultValue) {
        if (type == Type::List) {
            return _listValue;
        }
        else {
            return defaultValue;
        }
    }

    const JsonList& listValue(JsonList&& defaultValue) const {
        if (type == Type::List) {
            return _listValue;
        }
        else {
            return defaultValue;
        }
    }

    const std::string& stringValue() {
        if (type == Type::String) {
            return _stringValue;
        }
        throw std::logic_error("Improper return type: string");
    }
    
    const std::string& stringValue(std::string&& defaultValue) {
        if (type == Type::String) {
            return _stringValue;
        }
        else {
            return defaultValue;
        }
    }

    float numberValue() {
        if (type == Type::Number) {
            return _numberValue;
        }
        throw std::logic_error("Improper return type: number");
    }
    
    float numberValue(float defaultValue) {
        if (type == Type::Number) {
            return _numberValue;
        }
        else {
            return defaultValue;
        }
    }
    
    double doubleValue() {
        if (type == Type::Number) {
            return static_cast<double>(_numberValue);
        }
        throw std::logic_error("Improper return type: number");
    }
    
    double numberValue(double defaultValue) {
        if (type == Type::Number) {
            return static_cast<double>(_numberValue);
        }
        else {
            return defaultValue;
        }
    }
    
    int32_t intValue() {
        if (type == Type::Number) {
            return static_cast<int32_t>(_numberValue);
        }
        throw std::logic_error("Improper return type: number");
    }
    
    int32_t intValue(int32_t defaultValue) {
        if (type == Type::Number) {
            return static_cast<int32_t>(_numberValue);
        }
        else {
            return defaultValue;
        }
    }
    
    uint32_t uintValue() {
        if (type == Type::Number) {
            return static_cast<uint32_t>(_numberValue);
        }
        throw std::logic_error("Improper return type: number");
    }
    
    uint32_t uintValue(uint32_t defaultValue) {
        if (type == Type::Number) {
            return static_cast<uint32_t>(_numberValue);
        }
        else {
            return defaultValue;
        }
    }

    bool boolValue(bool defaultValue) {
        if (type == Type::Bool) {
            return _boolValue;
        }
        else {
            return defaultValue;
        }
    }

    void setValue(const JsonObject& object) {
        _objectValue = object;
        type = Type::Object;
    }
    
    void setValue(JsonObject&& object) {
        _objectValue = std::move(object);
        type = Type::Object;
    }

    void setValue(const JsonList& list) {
        _listValue = list;
        type = Type::List;
    }
    
    void setValue(JsonList&& list) {
        _listValue = std::move(list);
        type = Type::List;
    }

    void setValue(const char* str) {
        _stringValue = std::string(str);
        type = Type::String;
    }
    
    void setValue(const std::string& str) {
        _stringValue = std::move(str);
        type = Type::String;
    }
    
    void setValue(std::string&& str) {
        _stringValue = std::move(str);
        type = Type::String;
    }
    
    void setValue(char str) {
        _stringValue = std::to_string(str);
        type = Type::String;
    }
    
    void setValue(int32_t n) {
        _numberValue = static_cast<float>(n);
        type = Type::Number;
    }
    
    void setValue(uint32_t n) {
        _numberValue = static_cast<float>(n);
        type = Type::Number;
    }
    
    void setValue(float n) {
        _numberValue = n;
        type = Type::Number;
    }

    void setValue(double n) {
        _numberValue = static_cast<float>(n);
        type = Type::Number;
    }

    void setValue(bool b) {
        _boolValue = b;
        type = Type::Bool;
    }
    
    void setNull() {
        type = Type::Null;
    }

    bool isObject() {
        return type == Type::Object;
    }

    bool isList() {
        return type == Type::List;
    }

    bool isString() {
        return type == Type::String;
    }

    bool isNumber() {
        return type == Type::Number;
    }

    bool isBool() {
        return type == Type::Bool;
    }

    bool isNull() {
        return type == Type::Null;
    }
    
    // glm object functions
    glm::vec2 vec2Value(const glm::vec2& defaultValue = glm::vec2{0.0f, 0.0f}) {
        if (type == Type::List) {
            auto val = this->listValue();
            if (val.size() >= 2) {
                glm::vec2 result(val[0]->numberValue(), val[1]->numberValue());
                return result;
            }
        }
        return defaultValue;
    }
    
    glm::vec3 vec3Value(const glm::vec3& defaultValue = glm::vec3{0.0f, 0.0f, 0.0f}) {
        if (type == Type::List) {
            auto val = this->listValue();
            if (val.size() >= 3) {
                glm::vec3 result(val[0]->numberValue(), val[1]->numberValue(), val[2]->numberValue());
                return result;
            }
        }
        return defaultValue;
    }
    
    glm::vec4 vec4Value(const glm::vec4& defaultValue = glm::vec4{0.0f, 0.0f, 0.0f, 0.0f}) {
        if (type == Type::List) {
            auto val = this->listValue();
            if (val.size() >= 3) {
                glm::vec4 result(val[0]->numberValue(), val[1]->numberValue(), val[2]->numberValue(), val[3]->numberValue());
                return result;
            }
        }
        return defaultValue;
    }
    
    glm::mat4 mat4Value(const glm::mat4& defaultValue = glm::mat4{}) {
        if (type == Type::List) {
            auto val = this->listValue();
            if (val.size() >= 16) {
                // TODO: glm is column major, maybe this must be transposed?
                glm::mat4 result(
                    val[ 0]->numberValue(), val[ 1]->numberValue(), val[ 2]->numberValue(), val[ 3]->numberValue(),
                    val[ 4]->numberValue(), val[ 5]->numberValue(), val[ 6]->numberValue(), val[ 7]->numberValue(),
                    val[ 8]->numberValue(), val[ 9]->numberValue(), val[10]->numberValue(), val[11]->numberValue(),
                    val[12]->numberValue(), val[13]->numberValue(), val[14]->numberValue(), val[15]->numberValue());
                return result;
            }
        }
        return defaultValue;
    }
    
    void setValue(const glm::vec2& value) {
        setValue(JsonList{
            std::make_shared<JsonNode>(value.x),
            std::make_shared<JsonNode>(value.y)
        });
    }
    
    void setValue(const glm::vec3& value) {
        setValue(JsonList{
            std::make_shared<JsonNode>(value.x),
            std::make_shared<JsonNode>(value.y),
            std::make_shared<JsonNode>(value.z)
        });
    }
    
    void setValue(const glm::vec4& value) {
        setValue(JsonList{
            std::make_shared<JsonNode>(value.x),
            std::make_shared<JsonNode>(value.y),
            std::make_shared<JsonNode>(value.z),
            std::make_shared<JsonNode>(value.w)
        });
    }
    
    void setValue(const glm::mat4& value) {
        // TODO: glm is column major, maybe this must be transposed?
        setValue(JsonList{
            std::make_shared<JsonNode>(value[0][0]),
            std::make_shared<JsonNode>(value[0][1]),
            std::make_shared<JsonNode>(value[0][2]),
            std::make_shared<JsonNode>(value[0][3]),
            
            std::make_shared<JsonNode>(value[1][0]),
            std::make_shared<JsonNode>(value[1][1]),
            std::make_shared<JsonNode>(value[1][2]),
            std::make_shared<JsonNode>(value[1][3]),
            
            std::make_shared<JsonNode>(value[2][0]),
            std::make_shared<JsonNode>(value[2][1]),
            std::make_shared<JsonNode>(value[2][2]),
            std::make_shared<JsonNode>(value[2][3]),
            
            std::make_shared<JsonNode>(value[3][0]),
            std::make_shared<JsonNode>(value[3][1]),
            std::make_shared<JsonNode>(value[3][2]),
            std::make_shared<JsonNode>(value[3][3])
        });
    }

    void printNode(int indentationLevel = 0);

    std::string toString(int indentationLevel = 0);
};

extern BG2E_EXPORT std::shared_ptr<JsonNode> JSON(const JsonObject& p);
extern BG2E_EXPORT std::shared_ptr<JsonNode> JSON(JsonObject&& p);
extern BG2E_EXPORT std::shared_ptr<JsonNode> JSON(const JsonList& p);
extern BG2E_EXPORT std::shared_ptr<JsonNode> JSON(JsonList&& p);
extern BG2E_EXPORT std::shared_ptr<JsonNode> JSON(const char* p);
extern BG2E_EXPORT std::shared_ptr<JsonNode> JSON(std::string&& p);
extern BG2E_EXPORT std::shared_ptr<JsonNode> JSON(const std::string& p);
extern BG2E_EXPORT std::shared_ptr<JsonNode> JSON(char p);
extern BG2E_EXPORT std::shared_ptr<JsonNode> JSON(int32_t p);
extern BG2E_EXPORT std::shared_ptr<JsonNode> JSON(uint32_t p);
extern BG2E_EXPORT std::shared_ptr<JsonNode> JSON(float p);
extern BG2E_EXPORT std::shared_ptr<JsonNode> JSON(double p);
extern BG2E_EXPORT std::shared_ptr<JsonNode> JSON(bool p);
extern BG2E_EXPORT std::shared_ptr<JsonNode> JSON(const glm::vec2&);
extern BG2E_EXPORT std::shared_ptr<JsonNode> JSON(const glm::vec3&);
extern BG2E_EXPORT std::shared_ptr<JsonNode> JSON(const glm::vec4&);
extern BG2E_EXPORT std::shared_ptr<JsonNode> JSON(const glm::mat4&);
}
}

#endif
