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
#include <bg2e/draw/Drawable.hpp>

namespace bg2e::draw {
class RenderQueue;
class DrawableUsageLease;
}
namespace bg2e::manipulation {
class GizmoAndSelectionRenderer;
}

namespace bg2e::scene {

// Shared renderable view. Model references, ownership and geometry replacement
// belong to concrete components, never to this contract.
class BG2E_API DrawableRenderComponent : public Component {
  public:
    virtual ~DrawableRenderComponent() = default;

    // Legacy owner/text view. Instances return no Drawable through this bridge:
    // even a const Drawable exposes mutable mesh storage. Use the read contract.
    virtual std::shared_ptr<const draw::Drawable> drawable() const = 0;

    virtual bool isSceneGeometry() const
    {
        return true;
    }
    virtual bool renderable(bool /*forPicking*/ = false) const
    {
        return true;
    }
    virtual bool rayTracingEnabled() const
    {
        const auto resource = renderResource();
        return resource && resource->rayTracingEnabled();
    }

    virtual std::shared_ptr<const draw::Drawable::Mesh> geometry() const
    {
        const auto resource = renderResource();
        return resource ? resource->mesh() : nullptr;
    }
    virtual uint32_t submeshesCount() const
    {
        const auto resource = renderResource();
        return resource ? resource->submeshesCount() : 0;
    }
    virtual bool resourcesLoaded() const
    {
        const auto resource = renderResource();
        return resource && resource->isLoaded();
    }
    virtual draw::Engine* drawEngine() const
    {
        const auto resource = renderResource();
        return resource ? resource->engine() : nullptr;
    }
    virtual std::shared_ptr<const void> geometryIdentity() const
    {
        return renderResource();
    }
    virtual glm::mat4 transform() const
    {
        const auto resource = renderResource();
        return resource ? resource->transform() : glm::mat4(1);
    }
    virtual const std::string& submeshName(uint32_t index) const
    {
        return renderResource()->submeshName(index);
    }
    virtual const std::string& submeshGroupName(uint32_t index) const
    {
        return renderResource()->submeshGroupName(index);
    }
    virtual const base::MaterialAttributes& effectiveMaterial(uint32_t index) const
    {
        return renderResource()->material(index);
    }
    virtual std::shared_ptr<draw::MaterialBase> drawMaterial(uint32_t index) const
    {
        return renderResource()->drawMaterial(index);
    }
    virtual glm::mat4 submeshTransform(uint32_t index) const
    {
        return renderResource()->submeshTransform(index);
    }
    virtual bool submeshVisibility(uint32_t index) const
    {
        return renderResource()->submeshVisibility(index);
    }

  protected:
    friend class draw::RenderQueue;
    friend class manipulation::GizmoAndSelectionRenderer;

    virtual std::shared_ptr<const draw::Drawable> renderResource() const
    {
        return drawable();
    }
    virtual std::shared_ptr<draw::DrawableUsageLease> retainFrameResources() const
    {
        return nullptr;
    }
};

}
