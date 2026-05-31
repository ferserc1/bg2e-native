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

#include <bg2e/render/vulkan/rt/CollectRayTracingInstancesVisitor.hpp>
#include <bg2e/render/vulkan/rt/utils.hpp>
#include <bg2e/scene/TransformComponent.hpp>
#include <bg2e/scene/Node.hpp>
#include <bg2e/scene/Drawable.hpp>

namespace bg2e {
namespace render {
namespace vulkan {
namespace rt {

CollectRayTracingInstancesVisitor::CollectRayTracingInstancesVisitor(Engine *engine)
    : _engine {engine}
{
    _ignoreDisabled = true;
}

void CollectRayTracingInstancesVisitor::visit(scene::Node * node)
{
    auto transformComponent = node->transform();
    auto drawableComp = node->drawable();
    auto drw = drawableComp ? drawableComp->drawable() : nullptr;
    if (transformComponent)
    {
        _transformStack.push(_currentTransform);
        _currentTransform = _currentTransform * transformComponent->matrix();
    }

    if (drw && _engine->rayTracingSupported() && drw->rayTracingEnabled())
    {
        for (uint32_t i = 0; i < drw->submeshesCount(); ++i)
        {
            if (drw->submeshVisibility(i))
            {
                const auto & rtMesh = drw->rayTracingMesh(i);
                if (!rtMesh)
                {
                    std::cerr << "WARN: invalid rayTracingMesh found in submesh. Check Drawable initialization ("
                        << node->name() << " - "
                        << drw->name() << ")" << std::endl;
                    continue;
                }
                auto mat = mat4ToVkTransformMatrix(_currentTransform * drw->submeshTransform(i));
                auto renderMat = drw->renderMaterial(i);
                auto renderMesh = drw->renderMesh();

                RTMaterialInstance matInst {};
                matInst.data.albedo = renderMat->materialAttributes().albedo();
                matInst.data.albedoScale = renderMat->materialAttributes().albedoScale();
                matInst.vertexBuffer = renderMesh->vertexBuffer();
                matInst.indexBuffer = renderMesh->indexBuffer();
                matInst.albedoTexture = renderMat->albedoTexture().get();
                _materialInstances.push_back(matInst);

                uint32_t matIndex = static_cast<uint32_t>(_materialInstances.size() - 1);
                _instances.push_back({
                    .transform = mat,
                    .instanceCustomIndex = matIndex,
                    .mask = 0xFF,
                    .instanceShaderBindingTableRecordOffset = 0,
                    .flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR,
                    .accelerationStructureReference = rtMesh->deviceAddress()
                });
            }
        }
    }
}

void CollectRayTracingInstancesVisitor::didVisit(scene::Node * node)
{
    auto transformComponent = node->getComponent<scene::TransformComponent>();

    if (transformComponent)
    {
        _currentTransform = _transformStack.top();
        _transformStack.pop();
    }
}

}
}
}
}
