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
#include <bg2e/base/Color.hpp>

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

    void setEnvironmentImage(const std::string& img);

    inline void setEnvironmentImage(const std::filesystem::path& resourcePath, const std::string& img)
    {
        auto fullPath = resourcePath;
        fullPath.append(img);
        setEnvironmentImage(fullPath.string());
    }
    inline const std::string& environmentImage() const
    {
        return _environmentImage;
    }

    inline size_t imgHash() const
    {
        return _imgHash;
    }

    // Declarative draw sky settings; normalized blur selects GGX roughness.
    void setSkyboxBlur(float blur);
    float skyboxBlur() const
    {
        return _skyboxBlur;
    }
    void setSkyboxVisible(bool visible)
    {
        _skyboxVisible = visible;
    }
    bool skyboxVisible() const
    {
        return _skyboxVisible;
    }

    const base::Color& skyHorizonColor() const
    {
        return _skyHorizonColor;
    }
    void setSkyHorizonColor(const base::Color& color);

    const base::Color& skyZenithColor() const
    {
        return _skyZenithColor;
    }
    void setSkyZenithColor(const base::Color& color);

    const base::Color& groundColor() const
    {
        return _groundColor;
    }
    void setGroundColor(const base::Color& color);

    std::shared_ptr<Component> clone() const override;

    void deserialize(std::shared_ptr<json::JsonNode> jsonData, const std::filesystem::path& basePath) override;
    std::shared_ptr<json::JsonNode> serializeReferences(const std::filesystem::path& basePath) override;

    std::shared_ptr<json::JsonNode> serialize(const std::filesystem::path& basePath) override;

  protected:
    std::string _environmentImage;
    size_t _imgHash = 0;
    float _skyboxBlur = 0.0f;
    bool _skyboxVisible = true;

    base::Color _skyHorizonColor{0.55f, 0.65f, 0.8f};
    base::Color _skyZenithColor{0.06f, 0.22f, 0.6f};
    base::Color _groundColor{0.04f, 0.035f, 0.025f};

    // TODO: Other environment parameters
};

}
}
