//
//  MaterialSerializer.hpp
#pragma once

#include <bg2e/common.hpp>
#include <bg2e/base/MaterialAttributes.hpp>

#include <string>
#include <vector>
#include <filesystem>

namespace bg2e {
namespace utils {


class BG2E_API MaterialSerializer {
public:
    bool deserializeMaterial(
        const std::string& jsonString,
        const std::filesystem::path& basePath,
        base::MaterialAttributes& result
    );
    
    bool deserializeMaterial(
        const char * jsonString,
        const std::filesystem::path& basePath,
        base::MaterialAttributes& result
    ) {
        return deserializeMaterial(std::string(jsonString), basePath, result);
    }
    
    bool deserializeMaterialArray(
        const std::string& jsonString,
        const std::filesystem::path& basePath,
        std::vector<base::MaterialAttributes>& result
    );
    
    bool deserializeMaterialArray(
        const char* jsonString,
        const std::filesystem::path& basePath,
        std::vector<base::MaterialAttributes>& result
    ) {
        return deserializeMaterialArray(std::string(jsonString), basePath, result);
    }
    
    std::string serializeMaterial(
        base::MaterialAttributes& mat,
        std::vector<base::Texture*> & uniqueTextures
    );
    
    std::string serializeMaterialArray(
        std::vector<base::MaterialAttributes>& mat,
        std::vector<base::Texture*> & uniqueTextures
    );
};

}
}
