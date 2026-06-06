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

#include <bg2e/scene/Component.hpp>

#include <string>
#include <filesystem>


namespace bg2e {
namespace scene {

class BG2E_API EnvironmentComponent : public Component {
public:
    BG2E_COMPONENT_TYPE_NAME("Environment");

    EnvironmentComponent();
    EnvironmentComponent(const std::string& img);
    EnvironmentComponent(const std::filesystem::path& resourcePath, const std::string& file);
    virtual ~EnvironmentComponent();
    
    inline void setEnvironmentImage(const std::string& img)
    {
        _environmentImage = img;
        _imgHash = std::hash<std::string>{}(img);
    }
    
    inline void setEnvironmentImage(const std::filesystem::path& resourcePath, const std::string& img)
    {
        auto fullPath = resourcePath;
        fullPath.append(img);
        setEnvironmentImage(fullPath.string());
    }
    inline const std::string& environmentImage() const { return _environmentImage; }
    
    inline size_t imgHash() const { return _imgHash; }
    
    void deserialize(std::shared_ptr<json::JsonNode> jsonData, const std::filesystem::path& basePath, render::Engine& engine) override;
    std::shared_ptr<json::JsonNode> serialize(const std::filesystem::path& basePath) override;

protected:
    std::string _environmentImage;
    size_t _imgHash = 0;
    

    
    // TODO: Other environment parameters
};

}
}
