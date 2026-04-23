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

#include <bg2e/scene/Mesh.hpp>
#include <bg2e/base/MaterialAttributes.hpp>
#include <bg2e/render/Engine.hpp>
#include <bg2e/render/MaterialBase.hpp>
#include <bg2e/render/vulkan/geo/Mesh.hpp>
#include <bg2e/geo/modifiers.hpp>
#include <bg2e/scene/Component.hpp>
#include <bg2e/render/vulkan/rt/RayTracingMesh.hpp>

#include <memory>
#include <functional>
#include <vector>

namespace bg2e {
namespace scene {
        
class BG2E_API DrawableBase {
public:
    typedef std::function<std::vector<VkDescriptorSet>(
            bg2e::render::MaterialBase* material,
            const glm::mat4& transform,
            uint32_t submeshIndex
    )> DrawFunction;
    
    typedef std::function<std::vector<VkDescriptorSet>(
            bg2e::render::MaterialBase* material,
            const glm::mat4& transform
    )> DrawSubmeshFunction;
    
    DrawableBase() = default;
    virtual ~DrawableBase() = default;


    virtual void drawSubmesh(
        VkCommandBuffer cmd,
        VkPipelineLayout layout,
        uint32_t submesh,
        DrawSubmeshFunction cb,
        VkPipelineBindPoint bp
    ) = 0;
    
    virtual void draw(
        VkCommandBuffer cmd,
        VkPipelineLayout layout,
        DrawFunction cb,
        VkPipelineBindPoint bp
    ) = 0;
    
    inline void setTransform(const glm::mat4& mat)
    {
        _transform = mat;
    }
    
    [[nodiscard]] inline const glm::mat4& transform() const
    {
        return _transform;
    }
    
    inline glm::mat4& transform()
    {
        return _transform;
    }
    
    [[nodiscard]] inline const std::string& name() const
    {
        return _name;
    }
    
    inline void setName(const std::string& name)
    {
        _name = name;
    }
    
protected:
    // This transformation is applied to the whole mesh
    glm::mat4 _transform { 1.0f };
    std::string _name;
};

template <typename MeshT, typename RenderMeshT>
class BG2E_API DrawableGeneric : public DrawableBase {
public:
    DrawableGeneric() = default;
    DrawableGeneric(const DrawableGeneric&) = delete;
    DrawableGeneric& operator=(const DrawableGeneric&) = delete;
    ~DrawableGeneric() override = default;
    
    struct SubmeshAttributes {
        base::MaterialAttributes material;
        glm::mat4 transform { 1.0f };
        std::string name;
        std::string groupName;
        bool visible = true;
    };

    void applyModifier(geo::Modifier<MeshT> * mod);

    void setMesh(MeshT* mesh);
    void setMesh(std::shared_ptr<MeshT> mesh);
    const std::shared_ptr<MeshT>& mesh() const;
    
    void setMaterial(const base::MaterialAttributes& mat, uint32_t submeshIndex = 0);
    void setSubmeshTransform(const glm::mat4& mat, uint32_t submeshIndex = 0);
    void setSubmeshName(const std::string & name, uint32_t submeshIndex = 0);
    void setSubmeshGroupName(const std::string& groupName, uint32_t submeshIndex = 0);
    void setSubmeshVisibility(bool visible, uint32_t submeshIndex = 0);
    
    [[nodiscard]] const base::MaterialAttributes& material(uint32_t index = 0) const;
    [[nodiscard]] base::MaterialAttributes& material(uint32_t index = 0);
    [[nodiscard]] glm::mat4 submeshTransform(uint32_t index = 0) const;
    [[nodiscard]] std::string submeshName(uint32_t index = 0) const;
    [[nodiscard]] std::string submeshGroupName(uint32_t index = 0) const;
    [[nodiscard]] bool submeshVisibility(uint32_t index = 0) const;
    
    [[nodiscard]] uint32_t submeshesCount() const;

    [[nodiscard]] bg2e::geo::Submesh submeshData(uint32_t index = 0) const;
    
    std::vector<std::shared_ptr<render::MaterialBase>> materials() { return _materials; }
    
    void iterateMaterials(std::function<void(base::MaterialAttributes&)> cb);

    void load(render::Engine * engine);
    [[nodiscard]] inline bool isLoaded() const { return _renderMesh.get() != nullptr; }
    void reload();
    
    // Call this function when a material property has changed after calling the load() method
    void updateMaterials();
    
    std::shared_ptr<RenderMeshT> renderMesh();
    [[nodiscard]] const std::shared_ptr<render::MaterialBase>& renderMaterial(uint32_t submeshIndex) const;
    std::shared_ptr<render::MaterialBase>& renderMaterial(uint32_t submeshIndex);

    inline void setRayTracingEnabled(bool enabled) { _rayTracingEnabled = enabled; }
    [[nodiscard]] inline bool rayTracingEnabled() const { return _rayTracingEnabled && _engine && _engine->rayTracingSupported(); }

    inline const std::vector<std::unique_ptr<bg2e::render::vulkan::rt::RayTracingMeshGeneric<MeshT>>>& rayTracingMeshes() const { return _rayTracingMeshes; }
    inline std::unique_ptr<bg2e::render::vulkan::rt::RayTracingMeshGeneric<MeshT>> & rayTracingMesh(uint32_t submesh) { return _rayTracingMeshes[submesh]; }
    inline const std::unique_ptr<bg2e::render::vulkan::rt::RayTracingMeshGeneric<MeshT>> & rayTracingMesh(uint32_t submesh) const { return _rayTracingMeshes[submesh]; }
    
    void drawSubmesh(
        VkCommandBuffer cmd,
        VkPipelineLayout layout,
        uint32_t submesh,
        DrawSubmeshFunction cb,
        VkPipelineBindPoint bp
    ) override;
    
    void draw(
        VkCommandBuffer cmd,
        VkPipelineLayout layout,
        DrawFunction cb,
        VkPipelineBindPoint bp
    ) override;
    
protected:
    std::shared_ptr<MeshT> _mesh;
    std::vector<SubmeshAttributes> _submeshAttributes;

    
    // Default material attributes. It is used only to return something if the
    // submesh attributes accessors are called with an out of bound index, because we don't
	// whant that the access to the attributes to crash the program.
    base::MaterialAttributes _defaultMaterial;
    std::string _defaultString;
    
    // Render resources
    render::Engine * _engine = nullptr;
    std::shared_ptr<RenderMeshT> _renderMesh;
    std::vector<std::shared_ptr<render::MaterialBase>> _materials;

    // Ray tracing resources
    std::vector<std::unique_ptr<render::vulkan::rt::RayTracingMeshGeneric<MeshT>>> _rayTracingMeshes;
    bool _rayTracingEnabled = true;

    void buildRenderMesh();
    void buildMaterials();
    void buildRayTracingMeshes();
};

typedef BG2E_API DrawableGeneric<bg2e::geo::MeshP, bg2e::render::vulkan::geo::MeshP> DrawableP;
typedef BG2E_API DrawableGeneric<bg2e::geo::MeshPN, bg2e::render::vulkan::geo::MeshPN> DrawablePN;
typedef BG2E_API DrawableGeneric<bg2e::geo::MeshPC, bg2e::render::vulkan::geo::MeshPC> DrawablePC;
typedef BG2E_API DrawableGeneric<bg2e::geo::MeshPU, bg2e::render::vulkan::geo::MeshPU> DrawablePU;
typedef BG2E_API DrawableGeneric<bg2e::geo::MeshPNU, bg2e::render::vulkan::geo::MeshPNU> DrawablePNU;
typedef BG2E_API DrawableGeneric<bg2e::geo::MeshPNC, bg2e::render::vulkan::geo::MeshPNC> DrawablePNC;
typedef BG2E_API DrawableGeneric<bg2e::geo::MeshPNUC, bg2e::render::vulkan::geo::MeshPNUC> DrawablePNUC;
typedef BG2E_API DrawableGeneric<bg2e::geo::MeshPNUT, bg2e::render::vulkan::geo::MeshPNUT> DrawablePNUT;
typedef BG2E_API DrawableGeneric<bg2e::geo::MeshPNUUT, bg2e::render::vulkan::geo::MeshPNUUT> DrawablePNUUT;

// Default mesh type
typedef BG2E_API DrawableGeneric<scene::Mesh, scene::RenderMesh> Drawable;

}
}
