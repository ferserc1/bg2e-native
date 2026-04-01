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

#include <bg2e/app/Preferences.hpp>
#include <bg2e/app/MainLoop.hpp>
#include <bg2e/base/PlatformTools.hpp>
#include <bg2e/json/JsonParser.hpp>

#include <fstream>

namespace bg2e::app {

Preferences::Preferences()
{
    initFilePath("preferences.json");
}

Preferences::Preferences(const std::string & scope)
{
    initFilePath("preferences_" + scope + ".json");
}

Preferences::Preferences(std::string && scope)
{
    initFilePath("preferences_" + scope + ".json");
}

Preferences::~Preferences()
{
    save();
}

void Preferences::load()
{
    if (std::filesystem::exists(_filePath))
    {
        std::ifstream inFile(_filePath);
        if (!inFile.is_open())
        {
            std::cerr << "WARN: Could not open preferences file at path \"" << _filePath << "\""  << std::endl;
        }
        else {
            inFile.seekg(0, std::ios::end);
            std::string content;
            content.resize(inFile.tellg());
            
            inFile.seekg(0, std::ios::beg);
            inFile.read(&content[0], content.size());
            json::JsonParser parser(content);
            _root = parser.parse();
            inFile.close();
            
            if (!_root) {
                std::cerr << "WARN: Error parsing preferences file at path \"" << _filePath << "\"" << std::endl;
            }
        }
    }
    
    if (!_root.get())
    {
        _root = json::JSON(json::JsonObject{});
    }
    
    _dirty = false;
}

void Preferences::save() const
{
    if (!_dirty) return;
    
    auto fileContent = _root->toString();
    std::ofstream outFile(_filePath);
    if (outFile.is_open())
    {
        outFile << fileContent;
        outFile.close();
    }
    else
    {
        std::cerr << "WARN: Could not save preferences file at path \"" << _filePath << "\"" << std::endl;
    }
    
    _dirty = false;
}


template int8_t Preferences::get<int8_t>(const std::string&, const int8_t&) const;
template int16_t Preferences::get<int16_t>(const std::string&, const int16_t&) const;
template int32_t Preferences::get<int32_t>(const std::string&, const int32_t&) const;
template int64_t Preferences::get<int64_t>(const std::string&, const int64_t&) const;

template uint8_t Preferences::get<uint8_t>(const std::string&, const uint8_t&) const;
template uint16_t Preferences::get<uint16_t>(const std::string&, const uint16_t&) const;
template uint32_t Preferences::get<uint32_t>(const std::string&, const uint32_t&) const;
template uint64_t Preferences::get<uint64_t>(const std::string&, const uint64_t&) const;

template float Preferences::get<float>(const std::string&, const float&) const;
template double Preferences::get<double>(const std::string&, const double&) const;

template <typename T>
T Preferences::get(const std::string & key, const T& defaultValue) const
{
    auto & prefs = _root->objectValue();
    if (prefs[key].get() && prefs[key]->isNumber())
    {
        return prefs[key]->numberValue(static_cast<T>(defaultValue));
    }
    prefs.erase(key);
    return defaultValue;
}

const std::string& Preferences::get(const std::string& key, std::string&& defaultValue) const
{
    auto & prefs = _root->objectValue();
    if (prefs[key].get() && prefs[key]->isString())
    {
        return prefs[key]->stringValue(defaultValue);
    }
    prefs.erase(key);
    return defaultValue;
}

template <>
std::string Preferences::get<std::string>(const std::string & key, const std::string & defaultValue) const
{
    auto & prefs = _root->objectValue();
    if (prefs[key].get() && prefs[key]->isString())
    {
        return prefs[key]->stringValue(defaultValue);
    }
    prefs.erase(key);
    return defaultValue;
}

template <>
bool Preferences::get<bool>(const std::string & key, const bool& defaultValue) const
{
    auto & prefs = _root->objectValue();
    if (prefs[key].get() && prefs[key]->isBool())
    {
        return prefs[key]->boolValue(defaultValue);
    }
    prefs.erase(key);
    return defaultValue;
}

template <>
std::array<float, 2> Preferences::get<std::array<float, 2>>(const std::string & key, const std::array<float, 2>& defaultValue) const
{
    auto & prefs = _root->objectValue();
    if (prefs[key].get() && prefs[key]->isVec2())
    {
        return prefs[key]->vec2Value(defaultValue);
    }
    prefs.erase(key);
    return defaultValue;
}

template <>
std::array<float, 3> Preferences::get<std::array<float, 3>>(const std::string & key, const std::array<float, 3> & defaultValue) const
{
    auto & prefs = _root->objectValue();
    if (prefs[key].get() && prefs[key]->isVec3())
    {
        return prefs[key]->vec3Value(defaultValue);
    }
    prefs.erase(key);
    return defaultValue;
}

template <>
std::array<float, 4> Preferences::get<std::array<float, 4>>(const std::string & key, const std::array<float, 4> & defaultValue) const
{
    auto & prefs = _root->objectValue();
    if (prefs[key].get() && prefs[key]->isVec4())
    {
        return prefs[key]->vec4Value(defaultValue);
    }
    prefs.erase(key);
    return defaultValue;
}

template <>
base::Color Preferences::get<base::Color>(const std::string & key, const base::Color& defaultValue) const
{
    auto & prefs = _root->objectValue();
    if (prefs[key].get() && prefs[key]->isColor())
    {
        return prefs[key]->colorValue(defaultValue);
    }
    prefs.erase(key);
    return defaultValue;
}

template <>
std::array<float, 16> Preferences::get<std::array<float, 16>>(const std::string & key, const std::array<float, 16>& defaultValue) const
{
    auto & prefs = _root->objectValue();
    if (prefs[key].get() && prefs[key]->isMat4())
    {
        return prefs[key]->mat4Value(defaultValue);
    }
    prefs.erase(key);
    return defaultValue;
}

template <>
glm::vec2 Preferences::get<glm::vec2>(const std::string & key, const glm::vec2& defaultValue) const
{
    auto & prefs = _root->objectValue();
    if (prefs[key].get() && prefs[key]->isVec2())
    {
        return prefs[key]->glmVec2Value(defaultValue);
    }
    prefs.erase(key);
    return defaultValue;
}

template <>
glm::vec3 Preferences::get<glm::vec3>(const std::string & key, const glm::vec3 & defaultValue) const
{
    auto & prefs = _root->objectValue();
    if (prefs[key].get() && prefs[key]->isVec3())
    {
        return prefs[key]->glmVec3Value(defaultValue);
    }
    prefs.erase(key);
    return defaultValue;
}

template <>
glm::vec4 Preferences::get<glm::vec4>(const std::string & key, const glm::vec4 & defaultValue) const
{
    auto & prefs = _root->objectValue();
    if (prefs[key].get() && prefs[key]->isVec4())
    {
        return prefs[key]->glmVec4Value(defaultValue);
    }
    prefs.erase(key);
    return defaultValue;
}

template <>
glm::mat4 Preferences::get<glm::mat4>(const std::string & key, const glm::mat4 & defaultValue) const
{
    auto & prefs = _root->objectValue();
    if (prefs[key].get() && prefs[key]->isMat4())
    {
        return prefs[key]->glmMat4Value(defaultValue);
    }
    prefs.erase(key);
    return defaultValue;
}




    
// Setters
template void Preferences::set<bool>(const std::string&, const bool&);

template void Preferences::set<int8_t>(const std::string&, const int8_t&);
template void Preferences::set<int16_t>(const std::string&, const int16_t&);
template void Preferences::set<int32_t>(const std::string&, const int32_t&);
template void Preferences::set<int64_t>(const std::string&, const int64_t&);

template void Preferences::set<uint8_t>(const std::string&, const uint8_t&);
template void Preferences::set<uint16_t>(const std::string&, const uint16_t&);
template void Preferences::set<uint32_t>(const std::string&, const uint32_t&);
template void Preferences::set<uint64_t>(const std::string&, const uint64_t&);

template void Preferences::set<float>(const std::string&, const float&);
template void Preferences::set<double>(const std::string&, const double&);

template void Preferences::set<std::array<float, 2>>(const std::string&, const std::array<float, 2>&);
template void Preferences::set<std::array<float, 3>>(const std::string&, const std::array<float, 3>&);
template void Preferences::set<std::array<float, 4>>(const std::string&, const std::array<float, 4>&);

template void Preferences::set<glm::vec2>(const std::string&, const glm::vec2&);
template void Preferences::set<glm::vec3>(const std::string&, const glm::vec3&);
template void Preferences::set<glm::vec4>(const std::string&, const glm::vec4&);

template void Preferences::set<base::Color>(const std::string&, const base::Color&);

template void Preferences::set<std::array<float, 16>>(const std::string&, const std::array<float, 16>&);

template void Preferences::set<glm::mat3>(const std::string&, const glm::mat3&);
template void Preferences::set<glm::mat4>(const std::string&, const glm::mat4&);

template void Preferences::set<std::string>(const std::string&, const std::string&);

template <typename T>
void Preferences::set(const std::string& key, const T& value)
{
    auto& prefs = _root->objectValue();
    auto newValue = json::JSON(value);
    prefs[key] = newValue;
    _dirty = true;
}

void Preferences::set(const std::string& key, const char* value)
{
    auto& prefs = _root->objectValue();
    prefs[key] = json::JSON(value);
    _dirty = true;
}

void Preferences::set(const std::string& key, std::string&& value)
{
    auto& prefs = _root->objectValue();
    prefs[key] = json::JSON(value);
    _dirty = true;
}

void Preferences::initFilePath(const std::string & fileName)
{
    auto baseDir = base::PlatformTools::settingsPath();
    _filePath = baseDir / fileName;
    load();
}

}
