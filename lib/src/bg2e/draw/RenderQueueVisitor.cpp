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

#include <bg2e/draw/RenderQueueVisitor.hpp>
#include <bg2e/scene/DrawableComponent.hpp>
#include <bg2e/scene/Node.hpp>
#include <bg2e/scene/LightComponent.hpp>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace bg2e::draw {
RenderQueueVisitor::RenderQueueVisitor()
{
    _ignoreDisabled = true;
}
void RenderQueueVisitor::enqueue(scene::Node* root, Engine& engine, RenderQueue& queue)
{
    if (!root || _queue)
    {
        throw std::invalid_argument("Draw extraction needs a root and cannot be reentrant");
    }
    RenderQueue next;
    next.beginFrame(engine);
    for (auto* ancestor = root->parent(); ancestor; ancestor = ancestor->parent())
    {
        if (_ignoreDisabled && ancestor->disabled())
        {
            queue = std::move(next);
            return;
        }
    }
    _world = root->parent() ? root->parent()->worldMatrix() : glm::mat4{1};
    _stack.clear();
    _queue = &next;
    try
    {
        root->accept(this);
        if (!_stack.empty())
        {
            throw std::logic_error("Cancelled draw extraction did not complete its traversal");
        }
    }
    catch (...)
    {
        _queue = nullptr;
        _stack.clear();
        _world = glm::mat4{1};
        throw;
    }
    _queue = nullptr;
    _stack.clear();
    _world = glm::mat4{1};
    queue = std::move(next);
}
void RenderQueueVisitor::visit(scene::Node* node)
{
    if (!_queue)
    {
        throw std::logic_error("Use RenderQueueVisitor::enqueue to initialize extraction");
    }
    _stack.push_back(_world);
    if (node->transform())
    {
        _world *= node->transform()->matrix();
    }
    if (auto* component = node->drawable())
    {
        if (component->executionMode() == scene::DrawableComponent::ExecutionMode::Production)
        {
            throw std::invalid_argument("Draw visitor cannot consume a production Drawable or alternate legacy mesh layout");
        }
        auto drawable = component->drawDrawable();
        if (drawable)
        {
            if (!drawable->isLoaded())
            {
                throw std::logic_error("Load draw resources before extraction");
            }
            for (uint32_t i = 0; i < drawable->submeshesCount(); ++i)
            {
                _queue->enqueue(*node, drawable, i, _world * drawable->submeshTransform(i));
            }
        }
        else if (!component->resourcePath().empty())
        {
            throw std::logic_error("Load unresolved draw resources before extraction");
        }
    }
    if (auto* component = node->light(); component && component->light().type() != base::Light::TypeDisabled)
    {
        auto& uniforms = _queue->lights();
        if (uniforms.count.x >= MaxLights)
        {
            throw std::length_error("Draw light ABI supports at most eight enabled lights");
        }
        const auto& light = component->light();
        if (light.type() != base::Light::TypeOmni && light.type() != base::Light::TypeSpot && light.type() != base::Light::TypeDirectional)
        {
            throw std::invalid_argument("Unsupported draw light type");
        }
        const auto direction = glm::mat3(_world) * glm::vec3(0, 0, 1);
        const float length = glm::length(direction);
        const auto position = _world * glm::vec4(0, 0, 0, 1);
        if (!std::isfinite(length) || length < 1e-12f)
        {
            throw std::invalid_argument("Light direction must be finite and nonzero");
        }
        auto& data = uniforms.lights[uniforms.count.x];
        data.positionIntensity = {glm::vec3(position), light.intensity()};
        const auto& color = light.color();
        data.color = {color.r, color.g, color.b, color.a};
        data.directionAngle = {direction / length, light.spotAngle()};
        data.parameters = {light.spotCutoff(), light.sourceSize(), 0, 0};
        data.flags = {uint32_t(light.type()), light.castShadows() ? 1u : 0u, light.shadowSamples(), light.affectsReflections() ? 1u : 0u};
        for (const auto& vector : {data.positionIntensity, data.color, data.directionAngle, data.parameters})
        {
            for (int i = 0; i < 4; ++i)
            {
                if (!std::isfinite(vector[i]))
                {
                    throw std::invalid_argument("Light uniforms must be finite");
                }
            }
        }
        ++uniforms.count.x;
    }
    if (auto* environment = node->environment(); environment && !_queue->environment())
    {
        auto intent = std::dynamic_pointer_cast<scene::EnvironmentComponent>(environment->clone());
        if (!intent)
        {
            throw std::logic_error("Environment clone must preserve its component type");
        }
        _queue->setEnvironment(std::move(intent));
    }
}
void RenderQueueVisitor::didVisit(scene::Node*)
{
    if (!_queue || _stack.empty())
    {
        throw std::logic_error("Unbalanced draw visitor traversal");
    }
    _world = _stack.back();
    _stack.pop_back();
}
}
