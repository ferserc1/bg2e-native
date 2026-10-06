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
#include <bg2e/json/NodeReader.hpp>

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
        // Preserve bytes so the file size matches the number of characters read.
        std::ifstream inFile(_filePath, std::ios::binary);
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
            content.resize(static_cast<size_t>(inFile.gcount()));
            json::JsonParser parser(content);
            _root = parser.parse();
            inFile.close();
            
            if (!_root) {
                std::cerr << "WARN: Error parsing preferences file at path \"" << _filePath << "\"" << std::endl;
            }
        }
    }
    
    if (!json::ObjectReader(_root).isValid())
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


template <typename T>
T Preferences::get(const std::string& key, const T& defaultValue) const
{
    json::ObjectReader reader(_root);
    if constexpr (std::is_integral_v<T>)
        return reader.getInteger<T>(key).value_or(defaultValue);
    else
    {
        auto value = reader.getNumber(key);
        return value ? static_cast<T>(*value) : defaultValue;
    }
}

std::string Preferences::get(const std::string& key, std::string&& defaultValue) const
{
    return json::ObjectReader(_root).getString(key).value_or(std::move(defaultValue));
}

template <>
std::string Preferences::get<std::string>(const std::string& key, const std::string& defaultValue) const
{
    return json::ObjectReader(_root).getString(key).value_or(defaultValue);
}

template <>
bool Preferences::get<bool>(const std::string& key, const bool& defaultValue) const
{
    return json::ObjectReader(_root).getBool(key).value_or(defaultValue);
}

template <>
std::array<float, 2> Preferences::get<std::array<float, 2>>(const std::string& key, const std::array<float, 2>& defaultValue) const
{
    return json::ObjectReader(_root).getVec2(key).value_or(defaultValue);
}

template <>
std::array<float, 3> Preferences::get<std::array<float, 3>>(const std::string& key, const std::array<float, 3>& defaultValue) const
{
    return json::ObjectReader(_root).getVec3(key).value_or(defaultValue);
}

template <>
std::array<float, 4> Preferences::get<std::array<float, 4>>(const std::string& key, const std::array<float, 4>& defaultValue) const
{
    return json::ObjectReader(_root).getVec4(key).value_or(defaultValue);
}

template <>
base::Color Preferences::get<base::Color>(const std::string& key, const base::Color& defaultValue) const
{
    return json::ObjectReader(_root).getColor(key).value_or(defaultValue);
}

template <>
std::array<float, 16> Preferences::get<std::array<float, 16>>(const std::string& key, const std::array<float, 16>& defaultValue) const
{
    return json::ObjectReader(_root).getMat4(key).value_or(defaultValue);
}

template <>
glm::vec2 Preferences::get<glm::vec2>(const std::string& key, const glm::vec2& defaultValue) const
{
    return json::ObjectReader(_root).getGlmVec2(key).value_or(defaultValue);
}

template <>
glm::vec3 Preferences::get<glm::vec3>(const std::string& key, const glm::vec3& defaultValue) const
{
    return json::ObjectReader(_root).getGlmVec3(key).value_or(defaultValue);
}

template <>
glm::vec4 Preferences::get<glm::vec4>(const std::string& key, const glm::vec4& defaultValue) const
{
    return json::ObjectReader(_root).getGlmVec4(key).value_or(defaultValue);
}

template <>
glm::mat4 Preferences::get<glm::mat4>(const std::string& key, const glm::mat4& defaultValue) const
{
    return json::ObjectReader(_root).getGlmMat4(key).value_or(defaultValue);
}


// Setters
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

// Instantiate and export the supported types after their template definitions.
template BG2E_API int8_t Preferences::get<int8_t>(const std::string&, const int8_t&) const;
template BG2E_API int16_t Preferences::get<int16_t>(const std::string&, const int16_t&) const;
template BG2E_API int32_t Preferences::get<int32_t>(const std::string&, const int32_t&) const;
template BG2E_API int64_t Preferences::get<int64_t>(const std::string&, const int64_t&) const;

template BG2E_API uint8_t Preferences::get<uint8_t>(const std::string&, const uint8_t&) const;
template BG2E_API uint16_t Preferences::get<uint16_t>(const std::string&, const uint16_t&) const;
template BG2E_API uint32_t Preferences::get<uint32_t>(const std::string&, const uint32_t&) const;
template BG2E_API uint64_t Preferences::get<uint64_t>(const std::string&, const uint64_t&) const;

template BG2E_API float Preferences::get<float>(const std::string&, const float&) const;
template BG2E_API double Preferences::get<double>(const std::string&, const double&) const;

template BG2E_API void Preferences::set<bool>(const std::string&, const bool&);

template BG2E_API void Preferences::set<int8_t>(const std::string&, const int8_t&);
template BG2E_API void Preferences::set<int16_t>(const std::string&, const int16_t&);
template BG2E_API void Preferences::set<int32_t>(const std::string&, const int32_t&);
template BG2E_API void Preferences::set<int64_t>(const std::string&, const int64_t&);

template BG2E_API void Preferences::set<uint8_t>(const std::string&, const uint8_t&);
template BG2E_API void Preferences::set<uint16_t>(const std::string&, const uint16_t&);
template BG2E_API void Preferences::set<uint32_t>(const std::string&, const uint32_t&);
template BG2E_API void Preferences::set<uint64_t>(const std::string&, const uint64_t&);

template BG2E_API void Preferences::set<float>(const std::string&, const float&);
template BG2E_API void Preferences::set<double>(const std::string&, const double&);

template BG2E_API void Preferences::set<std::array<float, 2>>(const std::string&, const std::array<float, 2>&);
template BG2E_API void Preferences::set<std::array<float, 3>>(const std::string&, const std::array<float, 3>&);
template BG2E_API void Preferences::set<std::array<float, 4>>(const std::string&, const std::array<float, 4>&);

template BG2E_API void Preferences::set<glm::vec2>(const std::string&, const glm::vec2&);
template BG2E_API void Preferences::set<glm::vec3>(const std::string&, const glm::vec3&);
template BG2E_API void Preferences::set<glm::vec4>(const std::string&, const glm::vec4&);

template BG2E_API void Preferences::set<base::Color>(const std::string&, const base::Color&);

template BG2E_API void Preferences::set<std::array<float, 16>>(const std::string&, const std::array<float, 16>&);

template BG2E_API void Preferences::set<glm::mat3>(const std::string&, const glm::mat3&);
template BG2E_API void Preferences::set<glm::mat4>(const std::string&, const glm::mat4&);

template BG2E_API void Preferences::set<std::string>(const std::string&, const std::string&);

void Preferences::initFilePath(const std::string & fileName)
{
    auto baseDir = base::PlatformTools::settingsPath();
    _filePath = baseDir / fileName;
    load();
}

}
