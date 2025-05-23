//
//  Drawable.hpp
#pragma once

#include <bg2e/scene/Mesh.hpp>
#include <bg2e/base/MaterialAttributes.hpp>
#include <bg2e/render/Vulkan.hpp>
#include <bg2e/render/MaterialBase.hpp>
#include <bg2e/render/vulkan/geo/Mesh.hpp>

#include <memory>

namespace bg2e {
namespace scene {

template <typename MeshT, typename RenderMeshT>
class BG2E_API DrawableGeneric {
public:
    virtual ~DrawableGeneric() {}
    
    struct SubmeshAttributes {
        base::MaterialAttributes material;
        glm::mat4 transform = glm::mat4 { 1.0f } ;
    };

    void setMesh(MeshT* mesh);
    void setMesh(std::shared_ptr<MeshT> mesh);
    const std::shared_ptr<MeshT>& mesh() const;
    
    void setMaterial(const base::MaterialAttributes& mat, uint32_t submeshIndex = 0);
    void setTransform(const glm::mat4& mat, uint32_t submeshIndex = 0);
    const base::MaterialAttributes& material(uint32_t index = 0) const;
    base::MaterialAttributes& material(uint32_t index = 0);
    const glm::mat4& transform(uint32_t index = 0) const;
    glm::mat4& transform(uint32_t index = 0);
    
    void load(render::Vulkan * vk);
    inline bool isLoaded() const { return _renderMesh.get() != nullptr; }
    
    // Call this function when a material property has changed after calling the load() method
    void updateMaterials();
    
    std::shared_ptr<RenderMeshT> renderMesh();
    const std::shared_ptr<render::MaterialBase>& renderMaterial(uint32_t submeshIndex);
    
    void drawSubmesh(
        VkCommandBuffer cmd,
        VkPipelineLayout layout,
        uint32_t submesh,
        std::function<std::vector<VkDescriptorSet>(render::MaterialBase *, const glm::mat4&)> cb,
        VkPipelineBindPoint bp = VK_PIPELINE_BIND_POINT_GRAPHICS
    );
    
    void draw(
        VkCommandBuffer cmd,
        VkPipelineLayout layout,
        std::function<std::vector<VkDescriptorSet>(render::MaterialBase *, const glm::mat4&, uint32_t submIndex)> cb,
        VkPipelineBindPoint bp = VK_PIPELINE_BIND_POINT_GRAPHICS
    );
    
protected:
    std::shared_ptr<MeshT> _mesh;
    std::vector<SubmeshAttributes> _submeshAttributes;
    
    // Default matrix and material attributes. They are used only to return something if the
    // submesh attributes accessors are called with an out of bound index
    base::MaterialAttributes _defaultMaterial;
    glm::mat4 _defaultTransform;
    
    // Render resources
    // TODO: create a render::Mesh cache to avoid duplicities loading a shared geo::Mesh in
    // several Drawable objects
    render::Vulkan * _vulkan;
    std::shared_ptr<RenderMeshT> _renderMesh;
    std::vector<std::shared_ptr<render::MaterialBase>> _materials;
    
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
