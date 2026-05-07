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

#pragma once

#include <bg2e/common.hpp>
#include <bg2e/math/base.hpp>
#include <bg2e/base/Color.hpp>

#include <string>
#include <map>
#include <vector>
#include <array>
#include <exception>
#include <stdexcept>
#include <memory>
#include <cstdint>


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
    JsonNode(int8_t);
    JsonNode(int16_t);
    JsonNode(int32_t);
    JsonNode(int64_t);
    JsonNode(uint8_t);
    JsonNode(uint16_t);
    JsonNode(uint32_t);
    JsonNode(uint64_t);
    JsonNode(float);
    JsonNode(double);
    JsonNode(bool);
    JsonNode(const base::Color&);
    JsonNode(const glm::vec2&);
    JsonNode(const glm::vec3&);
    JsonNode(const glm::vec4&);
    JsonNode(const glm::mat3&);
    JsonNode(const glm::mat4&);
    JsonNode(const std::array<float, 2>&);
    JsonNode(const std::array<float, 3>&);
    JsonNode(const std::array<float, 4>&);
    JsonNode(const std::array<float, 9>&);
    JsonNode(const std::array<float, 16>&);
    
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
    
    glm::vec2 glmVec2Value() {
        if (isVec2()) {
            return glm::vec2{
                listValue()[0]->numberValue(0.f),
                listValue()[1]->numberValue(0.f)
            };
        }
        throw std::logic_error("Improper return type: vec2");
    }
    
    glm::vec3 glmVec3Value() {
        if (isVec3()) {
            return glm::vec3{
                listValue()[0]->numberValue(0.f),
                listValue()[1]->numberValue(0.f),
                listValue()[2]->numberValue(0.f)
            };
        }
        throw std::logic_error("Improper return type: vec3");
    }
    
    glm::vec4 glmVec4Value() {
        if (isVec4()) {
            return glm::vec4{
                listValue()[0]->numberValue(0.f),
                listValue()[1]->numberValue(0.f),
                listValue()[2]->numberValue(0.f),
                listValue()[3]->numberValue(0.f)
            };
        }
        throw std::logic_error("Improper return type: vec4");
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
        throw std::logic_error("Improper return type: vec3");
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
        throw std::logic_error("Improper return type: vec4");
    }
    
    base::Color colorValue() {
        if (isVec4()) {
            return base::Color(
                listValue()[0]->numberValue(0.f),
                listValue()[1]->numberValue(0.f),
                listValue()[2]->numberValue(0.f),
                listValue()[3]->numberValue(0.f)
            );
        }
        throw std::logic_error("Improper return type: Color");
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
    
    glm::mat4 glmMat4Value() {
        if (isMat4()) {
            return glm::mat4{
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

    int8_t numberValue(int8_t defaultValue) {
        if (type == Type::Number) {
            return static_cast<int8_t>(_numberValue);
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
    
    int32_t numberValue(int32_t defaultValue) {
        if (type == Type::Number) {
            return static_cast<int32_t>(_numberValue);
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
    
    uint8_t numberValue(uint8_t defaultValue) {
        if (type == Type::Number) {
            return static_cast<uint8_t>(_numberValue);
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
    
    uint32_t numberValue(uint32_t defaultValue) {
        if (type == Type::Number) {
            return static_cast<uint32_t>(_numberValue);
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
    
    glm::vec2 glmVec2Value(const glm::vec2& defaultValue) {
        if (isVec2()) {
            return glm::vec2{
                listValue()[0]->numberValue(0.f),
                listValue()[1]->numberValue(0.f)
            };
        }
        else {
            return defaultValue;
        }
    }
    
    glm::vec3 glmVec3Value(const glm::vec3& defaultValue) {
        if (isVec3()) {
            return glm::vec3{
                listValue()[0]->numberValue(0.f),
                listValue()[1]->numberValue(0.f),
                listValue()[2]->numberValue(0.f)
            };
        }
        else {
            return defaultValue;
        }
    }
    
    glm::vec4 glmVec4Value(const glm::vec4 & defaultValue) {
        if (isVec4()) {
            return glm::vec4{
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
    
    base::Color colorValue(const base::Color& defaultValue) {
        if (isVec4()) {
            return base::Color(
                listValue()[0]->numberValue(0.f),
                listValue()[1]->numberValue(0.f),
                listValue()[2]->numberValue(0.f),
                listValue()[3]->numberValue(0.f)
            );
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
    
    glm::mat4 glmMat4Value(const glm::mat4& defaultValue) {
        if (isMat4()) {
            return glm::mat4{
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
    
    void setValue(int8_t n) {
        _numberValue = static_cast<float>(n);
        type = Type::Number;
    }
    
    void setValue(int16_t n) {
        _numberValue = static_cast<float>(n);
        type = Type::Number;
    }
    
    void setValue(int32_t n) {
        _numberValue = static_cast<float>(n);
        type = Type::Number;
    }
    
    void setValue(int64_t n) {
        _numberValue = static_cast<float>(n);
        type = Type::Number;
    }
    
    void setValue(uint8_t n) {
        _numberValue = static_cast<float>(n);
        type = Type::Number;
    }
    
    void setValue(uint16_t n) {
        _numberValue = static_cast<float>(n);
        type = Type::Number;
    }
    
    void setValue(uint32_t n) {
        _numberValue = static_cast<float>(n);
        type = Type::Number;
    }
    
    void setValue(uint64_t n) {
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
    
    void setValue(const base::Color & c) {
        type = Type::List;
        _listValue = JsonList {
            std::make_shared<JsonNode>(c.r),
            std::make_shared<JsonNode>(c.g),
            std::make_shared<JsonNode>(c.b),
            std::make_shared<JsonNode>(c.a)
        };
    }
    
    void setValue(const glm::vec2 & v) {
        type = Type::List;
        _listValue = JsonList {
            std::make_shared<JsonNode>(v.x),
            std::make_shared<JsonNode>(v.y)
        };
    }
    
    void setValue(const glm::vec3 & v) {
        type = Type::List;
        _listValue = JsonList {
            std::make_shared<JsonNode>(v.x),
            std::make_shared<JsonNode>(v.y),
            std::make_shared<JsonNode>(v.z)
        };
    }
    
    void setValue(const glm::vec4 & v) {
        type = Type::List;
        _listValue = JsonList {
            std::make_shared<JsonNode>(v.x),
            std::make_shared<JsonNode>(v.y),
            std::make_shared<JsonNode>(v.z),
            std::make_shared<JsonNode>(v.w)
        };
    }
    
    void setValue(const glm::mat3 & m) {
        type = Type::List;
        _listValue = JsonList {
            std::make_shared<JsonNode>(m[0].x),
            std::make_shared<JsonNode>(m[0].y),
            std::make_shared<JsonNode>(m[0].z),
            std::make_shared<JsonNode>(m[1].x),
            std::make_shared<JsonNode>(m[1].y),
            std::make_shared<JsonNode>(m[1].z),
            std::make_shared<JsonNode>(m[2].x),
            std::make_shared<JsonNode>(m[2].y),
            std::make_shared<JsonNode>(m[2].z)
        };
    }
    
    void setValue(const glm::mat4 & m) {
        type = Type::List;
        _listValue = JsonList {
            std::make_shared<JsonNode>(m[0].x),
            std::make_shared<JsonNode>(m[0].y),
            std::make_shared<JsonNode>(m[0].z),
            std::make_shared<JsonNode>(m[0].w),
            std::make_shared<JsonNode>(m[1].x),
            std::make_shared<JsonNode>(m[1].y),
            std::make_shared<JsonNode>(m[1].z),
            std::make_shared<JsonNode>(m[1].w),
            std::make_shared<JsonNode>(m[2].x),
            std::make_shared<JsonNode>(m[2].y),
            std::make_shared<JsonNode>(m[2].z),
            std::make_shared<JsonNode>(m[2].w),
            std::make_shared<JsonNode>(m[3].x),
            std::make_shared<JsonNode>(m[3].y),
            std::make_shared<JsonNode>(m[3].z),
            std::make_shared<JsonNode>(m[3].w)
        };
    }
    
    
    
    
    
    void setValue(const std::array<float, 2> & v) {
        type = Type::List;
        _listValue = JsonList {
            std::make_shared<JsonNode>(v[0]),
            std::make_shared<JsonNode>(v[1])
        };
    }
    
    void setValue(const std::array<float, 3> & v) {
        type = Type::List;
        _listValue = JsonList {
            std::make_shared<JsonNode>(v[0]),
            std::make_shared<JsonNode>(v[1]),
            std::make_shared<JsonNode>(v[2])
        };
    }
    
    void setValue(const std::array<float, 4> & v) {
        type = Type::List;
        _listValue = JsonList {
            std::make_shared<JsonNode>(v[0]),
            std::make_shared<JsonNode>(v[1]),
            std::make_shared<JsonNode>(v[2]),
            std::make_shared<JsonNode>(v[3])
        };
    }
    
    void setValue(const std::array<float, 9> & m) {
        type = Type::List;
        _listValue = JsonList {
            std::make_shared<JsonNode>(m[0]),
            std::make_shared<JsonNode>(m[1]),
            std::make_shared<JsonNode>(m[2]),
            std::make_shared<JsonNode>(m[3]),
            std::make_shared<JsonNode>(m[4]),
            std::make_shared<JsonNode>(m[5]),
            std::make_shared<JsonNode>(m[6]),
            std::make_shared<JsonNode>(m[7]),
            std::make_shared<JsonNode>(m[8])
        };
    }
    
    void setValue(const std::array<float, 16> & m) {
        type = Type::List;
        _listValue = JsonList {
            std::make_shared<JsonNode>(m[ 0]),
            std::make_shared<JsonNode>(m[ 1]),
            std::make_shared<JsonNode>(m[ 2]),
            std::make_shared<JsonNode>(m[ 3]),
            std::make_shared<JsonNode>(m[ 4]),
            std::make_shared<JsonNode>(m[ 5]),
            std::make_shared<JsonNode>(m[ 6]),
            std::make_shared<JsonNode>(m[ 7]),
            std::make_shared<JsonNode>(m[ 8]),
            std::make_shared<JsonNode>(m[ 9]),
            std::make_shared<JsonNode>(m[10]),
            std::make_shared<JsonNode>(m[11]),
            std::make_shared<JsonNode>(m[12]),
            std::make_shared<JsonNode>(m[13]),
            std::make_shared<JsonNode>(m[14]),
            std::make_shared<JsonNode>(m[15])
        };
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
    
    inline bool isColor() {
        return isVec4();
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
    std::string serialize();
};

extern BG2E_API std::shared_ptr<JsonNode> JSON(const JsonObject& p);
extern BG2E_API std::shared_ptr<JsonNode> JSON(JsonObject&& p);
extern BG2E_API std::shared_ptr<JsonNode> JSON(const JsonList& p);
extern BG2E_API std::shared_ptr<JsonNode> JSON(JsonList&& p);
extern BG2E_API std::shared_ptr<JsonNode> JSON(const char* p);
extern BG2E_API std::shared_ptr<JsonNode> JSON(std::string&& p);
extern BG2E_API std::shared_ptr<JsonNode> JSON(const std::string& p);
extern BG2E_API std::shared_ptr<JsonNode> JSON(char p);
extern BG2E_API std::shared_ptr<JsonNode> JSON(int8_t p);
extern BG2E_API std::shared_ptr<JsonNode> JSON(int16_t p);
extern BG2E_API std::shared_ptr<JsonNode> JSON(int32_t p);
extern BG2E_API std::shared_ptr<JsonNode> JSON(int64_t p);
extern BG2E_API std::shared_ptr<JsonNode> JSON(uint8_t p);
extern BG2E_API std::shared_ptr<JsonNode> JSON(uint16_t p);
extern BG2E_API std::shared_ptr<JsonNode> JSON(uint32_t p);
extern BG2E_API std::shared_ptr<JsonNode> JSON(uint64_t p);
extern BG2E_API std::shared_ptr<JsonNode> JSON(float p);
extern BG2E_API std::shared_ptr<JsonNode> JSON(double p);
extern BG2E_API std::shared_ptr<JsonNode> JSON(bool p);
extern BG2E_API std::shared_ptr<JsonNode> JSON(const base::Color & p);
extern BG2E_API std::shared_ptr<JsonNode> JSON(const glm::vec2 & p);
extern BG2E_API std::shared_ptr<JsonNode> JSON(const glm::vec3 & p);
extern BG2E_API std::shared_ptr<JsonNode> JSON(const glm::vec4 & p);
extern BG2E_API std::shared_ptr<JsonNode> JSON(const glm::mat3 & p);
extern BG2E_API std::shared_ptr<JsonNode> JSON(const glm::mat4 & p);
extern BG2E_API std::shared_ptr<JsonNode> JSON(const std::array<float, 2> & p);
extern BG2E_API std::shared_ptr<JsonNode> JSON(const std::array<float, 3> & p);
extern BG2E_API std::shared_ptr<JsonNode> JSON(const std::array<float, 4> & p);
extern BG2E_API std::shared_ptr<JsonNode> JSON(const std::array<float, 9> & p);
extern BG2E_API std::shared_ptr<JsonNode> JSON(const std::array<float, 16> & p);

}
}
