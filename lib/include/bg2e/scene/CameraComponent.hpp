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

#include <bg2e/base/Camera.hpp>
#include <bg2e/scene/Component.hpp>

namespace bg2e {
namespace scene {

class BG2E_API CameraComponent : public Component {
public:
    BG2E_COMPONENT_TYPE_NAME("Camera");
    
    CameraComponent();
    virtual ~CameraComponent();
    
    const base::Camera& camera() const { return _camera; }
    base::Camera& camera() { return _camera; }
    
    math::Projection * projection() { return _camera.projection(); }
    template <class T>
    T* projection() { return _camera.projection<T>(); }
    void setProjection(math::Projection * proj) { _camera.setProjection(proj); }
    void setProjection(std::shared_ptr<math::Projection> proj) { _camera.setProjection(proj); }
    const glm::mat4& projectionMatrix() const { return _camera.projectionMatrix(); }
    const glm::mat4& viewMatrix() const;
    
    void resizeViewport(const math::Viewport& vp) override;
    void update(float delta) override;

    std::shared_ptr<Component> clone() const override;

    void deserialize(std::shared_ptr<json::JsonNode> jsonData, const std::filesystem::path& basePath, render::Engine& engine) override;
    std::shared_ptr<json::JsonNode> serialize(const std::filesystem::path& basePath) override;
    
protected:
    base::Camera _camera;

    // This matrix is obtained from the node inverse view matrix.
    mutable glm::mat4 _viewMatrix{ 1.0f };
};


}
}
