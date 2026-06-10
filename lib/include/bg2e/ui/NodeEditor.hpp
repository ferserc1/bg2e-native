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
#include <bg2e/ui/LightEditor.hpp>
#include <bg2e/ui/PolarTransformControllerEditor.hpp>
#include <bg2e/ui/CameraSettings.hpp>
#include <functional>
#include <vector>

namespace bg2e {
namespace render {
    class Engine;
}
namespace scene {
    class Node;
    class TransformComponent;
    class DrawableComponent;
    class EnvironmentComponent;
    class LightComponent;
    class CameraComponent;
}
namespace ui {

class BG2E_API NodeEditor {
public:
    using ChangedCallback = std::function<void()>;

    void init(render::Engine * engine);

    void setNode(scene::Node * node);
    void setNodes(const std::vector<scene::Node*>& nodes);
    scene::Node * node() const { return _node; }

    void draw();

    void onChanged(ChangedCallback cb) { _onChanged = cb; }

protected:
    render::Engine * _engine = nullptr;
    scene::Node * _node = nullptr;
    size_t _selectionCount = 0;

    LightEditor _lightEditor;
    PolarTransformControllerEditor _polarEditor;
    CameraSettings _cameraSettings;

    ChangedCallback _onChanged;
    void notifyChanged() const;

    void drawComponentList();
    void drawTransformEditor(scene::TransformComponent * t);
    void drawDrawableEditor(scene::DrawableComponent * d);
    void drawEnvironmentEditor(scene::EnvironmentComponent * e);
    void drawLightEditor(scene::LightComponent * l);
    void drawCameraEditor(scene::CameraComponent * c);

    // Cached euler angles for stable editing (keyed by node pointer)
    scene::Node * _eulerCacheNode = nullptr;
    glm::vec3 _cachedEuler = {0.0f, 0.0f, 0.0f};
};

}
}
