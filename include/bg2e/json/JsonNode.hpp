#pragma once

#include <bg2e/common.hpp>

#include <string>
#include <map>
#include <vector>
#include <array>
#include <exception>
#include <stdexcept>
#include <memory>


namespace bg2e {
namespace json {

class JsonNode;

using JsonObject = std::map<std::string, std::shared_ptr<JsonNode>>;

using JsonList = std::vector<std::shared_ptr<JsonNode>>;

class BG2E_API JsonNode {
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
    static JsonNode s_nullValue;

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
    JsonNode(float);
    JsonNode(double);
    JsonNode(bool);
    virtual ~JsonNode();

    JsonNode& getNullValue()
    {
        return s_nullValue;
    }
    
    JsonNode& objectValue(const std::string& key)
    {
        if (isObject() && _objectValue.find(key) != _objectValue.end()) {
            return *_objectValue[key].get();
        }
        else {
            return getNullValue();
        }
    }

    JsonNode& objectValue(const char* key)
    {
        return objectValue(std::string(key));
    }

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

    const std::string& stringValue() {
        if (type == Type::String) {
            return _stringValue;
        }
        throw std::logic_error("Improper return type: string");
    }

    float numberValue() {
        if (type == Type::Number) {
            return _numberValue;
        }
        throw std::logic_error("Improper return type: number");
    }

    bool boolValue() {
        if (type == Type::Bool) {
            return _boolValue;
        }
        throw std::logic_error("Improper return type: boolean");
    }
    
    std::array<float,2> vec2Value() {
        if (isVec2()) {
            return std::array<float, 2>{
                listValue()[0]->numberValue(0.f),
                listValue()[1]->numberValue(0.f)
            };
        }
        throw std::logic_error("Improper return type: vec2");
    }
    
    std::array<float,3> vec3Value() {
        if (isVec3()) {
            return std::array<float, 3>{
                listValue()[0]->numberValue(0.f),
                listValue()[1]->numberValue(0.f),
                listValue()[2]->numberValue(0.f)
            };
        }
        throw std::logic_error("Improper return type: vec2");
    }
    
    std::array<float,4> vec4Value() {
        if (isVec4()) {
            return std::array<float, 4>{
                listValue()[0]->numberValue(0.f),
                listValue()[1]->numberValue(0.f),
                listValue()[2]->numberValue(0.f),
                listValue()[3]->numberValue(0.f)
            };
        }
        throw std::logic_error("Improper return type: vec2");
    }
    
    std::array<float,16> mat4Value() {
        if (isMat4()) {
            return std::array<float, 16>{
                listValue()[0]->numberValue(0.f),
                listValue()[1]->numberValue(0.f),
                listValue()[2]->numberValue(0.f),
                listValue()[3]->numberValue(0.f),
                
                listValue()[4]->numberValue(0.f),
                listValue()[5]->numberValue(0.f),
                listValue()[6]->numberValue(0.f),
                listValue()[7]->numberValue(0.f),
                
                listValue()[8]->numberValue(0.f),
                listValue()[9]->numberValue(0.f),
                listValue()[10]->numberValue(0.f),
                listValue()[11]->numberValue(0.f),
                
                listValue()[12]->numberValue(0.f),
                listValue()[13]->numberValue(0.f),
                listValue()[14]->numberValue(0.f),
                listValue()[15]->numberValue(0.f)
            };
        }
        throw std::logic_error("Improper return type: mat4");
    }
    
    const std::string& stringValue(const std::string& defaultValue) {
        if (type == Type::String) {
            return _stringValue;
        }
        else {
            return defaultValue;
        }
    }

    uint8_t numberValue(uint8_t defaultValue) {
        if (type == Type::Number) {
            return static_cast<uint8_t>(_numberValue);
        }
        else {
            return defaultValue;
        }
    }
    
    int8_t numberValue(int8_t defaultValue) {
        if (type == Type::Number) {
            return static_cast<int8_t>(_numberValue);
        }
        else {
            return defaultValue;
        }
    }
    
    uint16_t numberValue(uint16_t defaultValue) {
        if (type == Type::Number) {
            return static_cast<uint16_t>(_numberValue);
        }
        else {
            return defaultValue;
        }
    }
    
    int16_t numberValue(int16_t defaultValue) {
        if (type == Type::Number) {
            return static_cast<int16_t>(_numberValue);
        }
        else {
            return defaultValue;
        }
    }
    
    uint32_t numberValue(uint32_t defaultValue) {
        if (type == Type::Number) {
            return static_cast<uint32_t>(_numberValue);
        }
        else {
            return defaultValue;
        }
    }
    
    int32_t numberValue(int32_t defaultValue) {
        if (type == Type::Number) {
            return static_cast<int32_t>(_numberValue);
        }
        else {
            return defaultValue;
        }
    }
    
    uint64_t numberValue(uint64_t defaultValue) {
        if (type == Type::Number) {
            return static_cast<uint64_t>(_numberValue);
        }
        else {
            return defaultValue;
        }
    }
    
    int64_t numberValue(int64_t defaultValue) {
        if (type == Type::Number) {
            return static_cast<int64_t>(_numberValue);
        }
        else {
            return defaultValue;
        }
    }
    
    float numberValue(float defaultValue) {
        if (type == Type::Number) {
            return _numberValue;
        }
        else {
            return defaultValue;
        }
    }
    
    double numberValue(double defaultValue) {
        if (type == Type::Number) {
            return static_cast<double>(_numberValue);
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
    
    std::array<float,2> vec2Value(const std::array<float,2>& defaultValue) {
        if (isVec2()) {
            return std::array<float, 2>{
                listValue()[0]->numberValue(0.f),
                listValue()[1]->numberValue(0.f)
            };
        }
        else {
            return defaultValue;
        }
    }
    
    std::array<float,3> vec3Value(const std::array<float, 3>& defaultValue) {
        if (isVec3()) {
            return std::array<float, 3>{
                listValue()[0]->numberValue(0.f),
                listValue()[1]->numberValue(0.f),
                listValue()[2]->numberValue(0.f)
            };
        }
        else {
            return defaultValue;
        }
    }
    
    std::array<float,4> vec4Value(const std::array<float, 4>& defaultValue) {
        if (isVec4()) {
            return std::array<float, 4>{
                listValue()[0]->numberValue(0.f),
                listValue()[1]->numberValue(0.f),
                listValue()[2]->numberValue(0.f),
                listValue()[3]->numberValue(0.f)
            };
        }
        else {
            return defaultValue;
        }
    }
    
    std::array<float,16> mat4Value(const std::array<float, 16>& defaultValue) {
        if (isMat4()) {
            return std::array<float, 16>{
                listValue()[0]->numberValue(0.f),
                listValue()[1]->numberValue(0.f),
                listValue()[2]->numberValue(0.f),
                listValue()[3]->numberValue(0.f),
                
                listValue()[4]->numberValue(0.f),
                listValue()[5]->numberValue(0.f),
                listValue()[6]->numberValue(0.f),
                listValue()[7]->numberValue(0.f),
                
                listValue()[8]->numberValue(0.f),
                listValue()[9]->numberValue(0.f),
                listValue()[10]->numberValue(0.f),
                listValue()[11]->numberValue(0.f),
                
                listValue()[12]->numberValue(0.f),
                listValue()[13]->numberValue(0.f),
                listValue()[14]->numberValue(0.f),
                listValue()[15]->numberValue(0.f)
            };
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
    
    bool isVec2() {
        return type == Type::List && listValue().size() == 2 &&
            listValue()[0]->isNumber() &&
            listValue()[1]->isNumber();
    }
    
    bool isVec3() {
        return type == Type::List && listValue().size() == 3 &&
            listValue()[0]->isNumber() &&
            listValue()[1]->isNumber() &&
            listValue()[2]->isNumber();
    }
    
    bool isVec4() {
        return type == Type::List && listValue().size() == 4 &&
            listValue()[0]->isNumber() &&
            listValue()[1]->isNumber() &&
            listValue()[2]->isNumber() &&
            listValue()[3]->isNumber();
    }
    
    bool isMat4() {
        return type == Type::List && listValue().size() == 16 &&
            listValue()[0]->isNumber() &&
            listValue()[1]->isNumber() &&
            listValue()[2]->isNumber() &&
            listValue()[3]->isNumber() &&

            listValue()[4]->isNumber() &&
            listValue()[5]->isNumber() &&
            listValue()[6]->isNumber() &&
            listValue()[7]->isNumber() &&
            
            listValue()[8]->isNumber() &&
            listValue()[9]->isNumber() &&
            listValue()[10]->isNumber() &&
            listValue()[11]->isNumber() &&
            
            listValue()[12]->isNumber() &&
            listValue()[13]->isNumber() &&
            listValue()[14]->isNumber() &&
            listValue()[15]->isNumber();
    }

    bool isNull() {
        return type == Type::Null;
    }

    void printNode(int indentationLevel = 0);

    std::string toString(int indentationLevel = 0);
};

extern BG2E_API std::shared_ptr<JsonNode> JSON(const JsonObject& p);
extern BG2E_API std::shared_ptr<JsonNode> JSON(JsonObject&& p);
extern BG2E_API std::shared_ptr<JsonNode> JSON(const JsonList& p);
extern BG2E_API std::shared_ptr<JsonNode> JSON(JsonList&& p);
extern BG2E_API std::shared_ptr<JsonNode> JSON(const char* p);
extern BG2E_API std::shared_ptr<JsonNode> JSON(std::string&& p);
extern BG2E_API std::shared_ptr<JsonNode> JSON(const std::string& p);
extern BG2E_API std::shared_ptr<JsonNode> JSON(char p);
extern BG2E_API std::shared_ptr<JsonNode> JSON(int32_t p);
extern BG2E_API std::shared_ptr<JsonNode> JSON(float p);
extern BG2E_API std::shared_ptr<JsonNode> JSON(double p);
extern BG2E_API std::shared_ptr<JsonNode> JSON(bool p);

}
}
