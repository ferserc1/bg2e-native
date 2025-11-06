//
//  Preferences.hpp

#pragma once

#include <bg2e/common.hpp>
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

}
}
