//
//  Drawable.hpp
#pragma once

#include <bg2e/scene/Mesh.hpp>
#include <bg2e/base/MaterialAttributes.hpp>
#include <bg2e/render/Engine.hpp>
#include <bg2e/render/MaterialBase.hpp>
#include <bg2e/render/vulkan/geo/Mesh.hpp>
#include <bg2e/scene/Component.hpp>

#include <memory>
#include <functional>

namespace bg2e {
namespace scene {
        
class DrawableBase {
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
    
    DrawableBase() {}
    virtual ~DrawableBase() {}

    virtual void drawSubmesh(
        VkCommandBuffer cmd,
        VkPipelineLayout layout,
        uint32_t submesh,
        DrawSubmeshFunction cb,
        VkPipelineBindPoint bp = VK_PIPELINE_BIND_POINT_GRAPHICS
    ) = 0;
    
    virtual void draw(
        VkCommandBuffer cmd,
        VkPipelineLayout layout,
        DrawFunction cb,
        VkPipelineBindPoint bp = VK_PIPELINE_BIND_POINT_GRAPHICS
    ) = 0;
    
    inline void setTransform(const glm::mat4& mat)
    {
        _transform = mat;
    }
    
    inline const glm::mat4& transform() const
    {
        return _transform;
    }
    
    inline glm::mat4& transform()
    {
        return _transform;
    }
    
    inline const std::string& name() const
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
    virtual ~DrawableGeneric() {}
    
    struct SubmeshAttributes {
        base::MaterialAttributes material;
        glm::mat4 transform { 1.0f } ;
    };

    void setMesh(MeshT* mesh);
    void setMesh(std::shared_ptr<MeshT> mesh);
    const std::shared_ptr<MeshT>& mesh() const;
    
    void setMaterial(const base::MaterialAttributes& mat, uint32_t submeshIndex = 0);
    void setSubmeshTransform(const glm::mat4& mat, uint32_t submeshIndex = 0);
    const base::MaterialAttributes& material(uint32_t index = 0) const;
    base::MaterialAttributes& material(uint32_t index = 0);
    glm::mat4 submeshTransform(uint32_t index = 0) const;
    
    std::vector<std::shared_ptr<render::MaterialBase>> materials() { return _materials; }
    
    void iterateMaterials(std::function<void(base::MaterialAttributes&)> cb);
    
    void load(render::Engine * engine);
    inline bool isLoaded() const { return _renderMesh.get() != nullptr; }
    
    // Call this function when a material property has changed after calling the load() method
    void updateMaterials();
    
    std::shared_ptr<RenderMeshT> renderMesh();
    const std::shared_ptr<render::MaterialBase>& renderMaterial(uint32_t submeshIndex);
    
    void drawSubmesh(
        VkCommandBuffer cmd,
        VkPipelineLayout layout,
        uint32_t submesh,
        DrawSubmeshFunction cb,
        VkPipelineBindPoint bp = VK_PIPELINE_BIND_POINT_GRAPHICS
    );
    
    void draw(
        VkCommandBuffer cmd,
        VkPipelineLayout layout,
        DrawFunction cb,
        VkPipelineBindPoint bp = VK_PIPELINE_BIND_POINT_GRAPHICS
    );
    
protected:
    std::shared_ptr<MeshT> _mesh;
    std::vector<SubmeshAttributes> _submeshAttributes;

    
    // Default material attributes. It is used only to return something if the
    // submesh attributes accessors are called with an out of bound index, because we don't
	// whant that the access to the attributes to crash the program.
    base::MaterialAttributes _defaultMaterial;
    
    // Render resources
    // TODO: create a render::Mesh cache to avoid duplicities loading a shared geo::Mesh in
    // several Drawable objects
    render::Engine * _engine = nullptr;
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
