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

#include <bg2e/ui/NodeEditor.hpp>
#include <bg2e/ui/BasicWidgets.hpp>
#include <bg2e/ui/Input.hpp>
#include <bg2e/scene/Node.hpp>
#include <bg2e/scene/TransformComponent.hpp>
#include <bg2e/scene/DrawableComponent.hpp>
#include <bg2e/scene/EnvironmentComponent.hpp>
#include <bg2e/scene/LightComponent.hpp>
#include <bg2e/scene/CameraComponent.hpp>
#include <bg2e/scene/PolarTransformController.hpp>
#include <bg2e/db/mesh_bg2.hpp>
#include <bg2e/app/FileDialog.hpp>
#include <bg2e/app/MainLoop.hpp>
#include <bg2e/render/Engine.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>

#include <filesystem>

namespace bg2e::ui {

void NodeEditor::init(render::Engine * engine)
{
    _engine = engine;
}

void NodeEditor::setNode(scene::Node * node)
{
    _node = node;
    _selectionCount = node ? 1 : 0;
    _eulerCacheNode = nullptr;
}

void NodeEditor::setNodes(const std::vector<scene::Node*>& nodes)
{
    if (nodes.size() == 1)
    {
        _node = nodes.front();
        _selectionCount = 1;
    }
    else
    {
        _node = nullptr;
        _selectionCount = nodes.size();
    }
    _eulerCacheNode = nullptr;
}

void NodeEditor::draw()
{
    if (_selectionCount > 1)
    {
        BasicWidgets::text("<multiple_selection>");
        return;
    }

    if (!_node)
    {
        BasicWidgets::text("No selection");
        return;
    }

    auto name = _node->name();
    auto enabled = _node->enabled();
    if (Input::text("Node name", name))
    {
        _node->setName(name);
    }
    if (BasicWidgets::checkBox("Enabled", &enabled))
    {
        _node->setEnabled(enabled);
    }

    drawComponentList();

    if (auto t = _node->getComponent<scene::TransformComponent>())
    {
        drawTransformEditor(t);
    }
    if (auto d = _node->getComponent<scene::DrawableComponent>())
    {
        drawDrawableEditor(d);
    }
    if (auto e = _node->getComponent<scene::EnvironmentComponent>())
    {
        drawEnvironmentEditor(e);
    }
    if (auto l = _node->getComponent<scene::LightComponent>())
    {
        drawLightEditor(l);
    }
    if (auto c = _node->getComponent<scene::CameraComponent>())
    {
        drawCameraEditor(c);
    }
}

void NodeEditor::drawComponentList()
{
    BasicWidgets::separator("Components");
    for (auto & comp : _node->orderedComponents())
    {
        BasicWidgets::text(comp->typeName());
    }
}

void NodeEditor::drawTransformEditor(scene::TransformComponent * t)
{
    BasicWidgets::separator("Transform");

    const glm::mat4 & m = t->matrix();

    glm::vec3 pos = glm::vec3(m[3]);

    glm::vec3 scale;
    scale.x = glm::length(glm::vec3(m[0]));
    scale.y = glm::length(glm::vec3(m[1]));
    scale.z = glm::length(glm::vec3(m[2]));

    if (_eulerCacheNode != _node)
    {
        glm::mat3 rot;
        if (scale.x != 0.0f) rot[0] = glm::vec3(m[0]) / scale.x;
        else rot[0] = glm::vec3(1.0f, 0.0f, 0.0f);
        if (scale.y != 0.0f) rot[1] = glm::vec3(m[1]) / scale.y;
        else rot[1] = glm::vec3(0.0f, 1.0f, 0.0f);
        if (scale.z != 0.0f) rot[2] = glm::vec3(m[2]) / scale.z;
        else rot[2] = glm::vec3(0.0f, 0.0f, 1.0f);

        float xr, yr, zr;
        glm::extractEulerAngleXYZ(glm::mat4(rot), xr, yr, zr);
        _cachedEuler = glm::degrees(glm::vec3(xr, yr, zr));
        _eulerCacheNode = _node;
    }

    glm::vec3 eulerDeg = _cachedEuler;

    bool changed = false;
    if (Input::vec3("Position", pos)) changed = true;
    if (Input::vec3("Rotation", eulerDeg)) changed = true;
    if (Input::vec3("Scale", scale)) changed = true;

    if (changed)
    {
        _cachedEuler = eulerDeg;
        glm::vec3 rad = glm::radians(eulerDeg);
        glm::mat4 mNew = glm::translate(glm::mat4(1.0f), pos)
            * glm::eulerAngleXYZ(rad.x, rad.y, rad.z)
            * glm::scale(glm::mat4(1.0f), scale);
        t->setMatrix(mNew);
        notifyChanged();
    }
}

void NodeEditor::drawDrawableEditor(scene::DrawableComponent * d)
{
    BasicWidgets::separator("Drawable");

    if (BasicWidgets::button("Replace Model..."))
    {
        auto path = app::FileDialog::getOpenFilePath({{ "bg2e 3D model", "bg2,vwglb" }});
        if (!path.empty())
        {
            auto drawable = db::loadDrawableBg2(path, _engine);
            app::MainLoop::current()->safeUpdateScene([d, drawable]() {
                d->setDrawable(drawable);
            });
            notifyChanged();
        }
    }
}

void NodeEditor::drawEnvironmentEditor(scene::EnvironmentComponent * e)
{
    BasicWidgets::separator("Environment");

    auto imgPath = std::filesystem::path(e->environmentImage());
    BasicWidgets::text(imgPath.filename().string());

    if (BasicWidgets::button("Select Image..."))
    {
        auto path = app::FileDialog::getOpenFilePath({{ "HDR Environments", "hdr,jpg,png" }});
        if (!path.empty())
        {
            e->setEnvironmentImage(path.string());
            notifyChanged();
        }
    }
}

void NodeEditor::drawLightEditor(scene::LightComponent * l)
{
    BasicWidgets::separator("Light");

    _lightEditor.setLightComponent(
        std::static_pointer_cast<scene::LightComponent>(l->shared_from_this()));
    if (_lightEditor.draw())
    {
        notifyChanged();
    }

    if (auto polar = _node->getComponent<scene::PolarTransformControllerComponent>())
    {
        _polarEditor.setComponent(polar);
        if (_polarEditor.draw())
        {
            notifyChanged();
        }
    }
}

void NodeEditor::drawCameraEditor(scene::CameraComponent * c)
{
    BasicWidgets::separator("Camera");

    _cameraSettings.setCameraComponent(c);
    if (_cameraSettings.draw())
    {
        notifyChanged();
    }
}

void NodeEditor::notifyChanged() const
{
    if (_onChanged)
    {
        _onChanged();
    }
}

}
