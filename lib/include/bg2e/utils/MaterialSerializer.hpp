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
        std::vector<std::shared_ptr<base::Texture>> & uniqueTextures,
        bool relativePaths
    );
    
    std::string serializeMaterialArray(
        std::vector<base::MaterialAttributes>& mat,
        std::vector<std::shared_ptr<base::Texture>> & uniqueTextures,
        bool relativePaths
    );

protected:
    void addUniqueTexture(
        std::shared_ptr<base::Texture> tex,
        std::vector<std::shared_ptr<base::Texture>>& textures
    );
};

}
}
