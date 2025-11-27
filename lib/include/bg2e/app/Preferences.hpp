//
//  Preferences.hpp

#pragma once

#include <bg2e/common.hpp>
#include <bg2e/base/PlatformTools.hpp>
#include <bg2e/json/JsonNode.hpp>
#include <bg2e/base/Color.hpp>

#include <filesystem>
#include <memory>

namespace bg2e {
namespace app {

class BG2E_API Preferences {
public:
    Preferences();
    explicit Preferences(const std::string & scope);
    explicit Preferences(std::string && scope);
    ~Preferences();
    
    void load();
    
    void save() const;
    
    // Getters
    template <typename T>
    T get(const std::string & key, const T& defaultValue) const;
    const std::string& get(const std::string& key, std::string&& defaultValue) const;
        
    // Setters
    template <typename T>
    void set(const std::string & key, const T& value);
    
    void set(const std::string & key, const char* value);
    
    void set(const std::string & key, std::string && value);
    
protected:
    std::filesystem::path _filePath;
    std::shared_ptr<json::JsonNode> _root;
    
    mutable bool _dirty = false;
    
    void initFilePath(const std::string & fileName);
    
    std::string _emptyString = "";
};

#ifdef BG2E_IS_WINDOWS

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

template <>
BG2E_API std::string Preferences::get<std::string>(const std::string& key, const std::string& defaultValue) const;

template <>
BG2E_API bool Preferences::get<bool>(const std::string& key, const bool& defaultValue) const;

template <>
BG2E_API std::array<float, 2> Preferences::get<std::array<float, 2>>(const std::string& key, const std::array<float, 2>& defaultValue) const;

template <>
BG2E_API std::array<float, 3> Preferences::get<std::array<float, 3>>(const std::string& key, const std::array<float, 3>& defaultValue) const;

template <>
BG2E_API std::array<float, 4> Preferences::get<std::array<float, 4>>(const std::string& key, const std::array<float, 4>& defaultValue) const;

template <>
BG2E_API base::Color Preferences::get<base::Color>(const std::string& key, const base::Color& defaultValue) const;
template <>
BG2E_API std::array<float, 16> Preferences::get<std::array<float, 16>>(const std::string& key, const std::array<float, 16>& defaultValue) const;

template <>
BG2E_API glm::vec2 Preferences::get<glm::vec2>(const std::string& key, const glm::vec2& defaultValue) const;

template <>
BG2E_API glm::vec3 Preferences::get<glm::vec3>(const std::string& key, const glm::vec3& defaultValue) const;

template <>
BG2E_API glm::vec4 Preferences::get<glm::vec4>(const std::string& key, const glm::vec4& defaultValue) const;

template <>
BG2E_API glm::mat4 Preferences::get<glm::mat4>(const std::string& key, const glm::mat4& defaultValue) const;

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

#endif

}
}
