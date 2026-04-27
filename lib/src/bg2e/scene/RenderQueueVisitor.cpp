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

#include "../include/bg2e/scene/RenderQueueVisitor.hpp"
#include <bg2e/scene/Drawable.hpp>
#include <bg2e/scene/DrawableComponent.hpp>
#include <bg2e/scene/TransformComponent.hpp>
#include <bg2e/render/MaterialBase.hpp>
#include <bg2e/render/vulkan/geo/Mesh.hpp>
#include <bg2e/scene/Node.hpp>

#include <memory>
#include <glm/glm.hpp>

namespace bg2e {
namespace scene {

// Especialización explícita de enqueue

template <typename DrawableT>
RenderQueueVisitor<DrawableT>::RenderQueueVisitor()
{
    _ignoreDisabled = true;
}

template <typename DrawableT>
void RenderQueueVisitor<DrawableT>::enqueue(
    bg2e::scene::Node* sceneRoot,
    bg2e::render::RenderQueue<DrawableT>* renderQueue
) {
    _renderQueue = renderQueue;
    _renderQueue->beginFrame();
    sceneRoot->accept(this);
}

template <typename DrawableT>
void RenderQueueVisitor<DrawableT>::visit(bg2e::scene::Node* node) {
    auto drawableComponent = node->getComponent<bg2e::scene::DrawableComponent>();
    auto transformComponent = node->getComponent<bg2e::scene::TransformComponent>();
    if (transformComponent) {
        _transformStack.push(_currentTransform);
        _currentTransform = _currentTransform * transformComponent->matrix();
    }
    if (drawableComponent) {
        Drawable* drawable = dynamic_cast<Drawable*>(drawableComponent->drawableBase().get());
        if (drawable) {
            for (size_t i = 0; i < drawable->materials().size(); ++i) {
                if (drawable->submeshVisibility(static_cast<uint32_t>(i)))
                {
                    std::shared_ptr<render::MaterialBase> mat = drawable->renderMaterial(static_cast<uint32_t>(i));
                    auto submeshTransform = drawable->submeshTransform(static_cast<uint32_t>(i));
                    auto trx = _currentTransform * submeshTransform;
                    std::shared_ptr<render::vulkan::geo::Mesh> renderMesh = drawable->renderMesh();
                    _renderQueue->enqueue(renderMesh, static_cast<uint32_t>(i), mat, trx);
                }
            }
        }
    }
}

template <typename DrawableT>
void RenderQueueVisitor<DrawableT>::didVisit(bg2e::scene::Node* node) {
    auto transformComponent = node->getComponent<bg2e::scene::TransformComponent>();
    if (transformComponent) {
        _currentTransform = _transformStack.top();
        _transformStack.pop();
    }
}

// DrawableP
template RenderQueueVisitor<scene::DrawableP>::RenderQueueVisitor();
template void RenderQueueVisitor<scene::DrawableP>::enqueue(Node*, bg2e::render::RenderQueue<scene::DrawableP>*);
template void RenderQueueVisitor<scene::DrawableP>::visit(Node*);
template void RenderQueueVisitor<scene::DrawableP>::didVisit(Node*);

// DrawablePN
template RenderQueueVisitor<scene::DrawablePN>::RenderQueueVisitor();
template void RenderQueueVisitor<scene::DrawablePN>::enqueue(Node*, bg2e::render::RenderQueue<scene::DrawablePN>*);
template void RenderQueueVisitor<scene::DrawablePN>::visit(Node*);
template void RenderQueueVisitor<scene::DrawablePN>::didVisit(Node*);

// DrawablePC
template RenderQueueVisitor<scene::DrawablePC>::RenderQueueVisitor();
template void RenderQueueVisitor<scene::DrawablePC>::enqueue(Node*, bg2e::render::RenderQueue<scene::DrawablePC>*);
template void RenderQueueVisitor<scene::DrawablePC>::visit(Node*);
template void RenderQueueVisitor<scene::DrawablePC>::didVisit(Node*);

// DrawablePU
template RenderQueueVisitor<scene::DrawablePU>::RenderQueueVisitor();
template void RenderQueueVisitor<scene::DrawablePU>::enqueue(Node*, bg2e::render::RenderQueue<scene::DrawablePU>*);
template void RenderQueueVisitor<scene::DrawablePU>::visit(Node*);
template void RenderQueueVisitor<scene::DrawablePU>::didVisit(Node*);

// DrawablePNU
template RenderQueueVisitor<scene::DrawablePNU>::RenderQueueVisitor();
template void RenderQueueVisitor<scene::DrawablePNU>::enqueue(Node*, bg2e::render::RenderQueue<scene::DrawablePNU>*);
template void RenderQueueVisitor<scene::DrawablePNU>::visit(Node*);
template void RenderQueueVisitor<scene::DrawablePNU>::didVisit(Node*);

// DrawablePNC
template RenderQueueVisitor<scene::DrawablePNC>::RenderQueueVisitor();
template void RenderQueueVisitor<scene::DrawablePNC>::enqueue(Node*, bg2e::render::RenderQueue<scene::DrawablePNC>*);
template void RenderQueueVisitor<scene::DrawablePNC>::visit(Node*);
template void RenderQueueVisitor<scene::DrawablePNC>::didVisit(Node*);

// DrawablePNUC
template RenderQueueVisitor<scene::DrawablePNUC>::RenderQueueVisitor();
template void RenderQueueVisitor<scene::DrawablePNUC>::enqueue(Node*, bg2e::render::RenderQueue<scene::DrawablePNUC>*);
template void RenderQueueVisitor<scene::DrawablePNUC>::visit(Node*);
template void RenderQueueVisitor<scene::DrawablePNUC>::didVisit(Node*);

// DrawablePNUT
template RenderQueueVisitor<scene::DrawablePNUT>::RenderQueueVisitor();
template void RenderQueueVisitor<scene::DrawablePNUT>::enqueue(Node*, bg2e::render::RenderQueue<scene::DrawablePNUT>*);
template void RenderQueueVisitor<scene::DrawablePNUT>::visit(Node*);
template void RenderQueueVisitor<scene::DrawablePNUT>::didVisit(Node*);

// DrawablePNUUT
template RenderQueueVisitor<scene::DrawablePNUUT>::RenderQueueVisitor();
template void RenderQueueVisitor<scene::DrawablePNUUT>::enqueue(Node*, bg2e::render::RenderQueue<scene::DrawablePNUUT>*);
template void RenderQueueVisitor<scene::DrawablePNUUT>::visit(Node*);
template void RenderQueueVisitor<scene::DrawablePNUUT>::didVisit(Node*);



} // namespace scene
} // namespace bg2e


