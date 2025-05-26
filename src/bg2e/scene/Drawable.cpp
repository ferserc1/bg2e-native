//
//  Drawable.cpp

#include <bg2e/scene/Drawable.hpp>
#include <bg2e/base/Log.hpp>
#include <memory>

#define assert_submesh(i)       if (i >= _submeshAttributes.size()) { \
                                    bg2e_log_warning << "Submesh index out of bounds setting Drawable material. Submeshes=" << _submeshAttributes.size() << ", Index=" << i << bg2e_log_end; \
                                    return;\
                                }

#define assert_submesh_r(i, r)  if (i >= _submeshAttributes.size()) { \
                                    bg2e_log_warning << "Submesh index out of bounds setting Drawable material. Submeshes=" << _submeshAttributes.size() << ", Index=" << i << bg2e_log_end; \
                                    return r;\
                                }

namespace bg2e::scene {

template <typename MeshT, typename RenderMeshT>
void DrawableGeneric<MeshT, RenderMeshT>::setMesh(MeshT* mesh)
{
    setMesh(std::shared_ptr<MeshT>(mesh));
}

template <typename MeshT, typename RenderMeshT>
void DrawableGeneric<MeshT, RenderMeshT>::setMesh(std::shared_ptr<MeshT> mesh)
{
    _mesh = mesh;
    _submeshAttributes.clear();
    _submeshAttributes.resize(mesh->submeshes.size());
}

template <typename MeshT, typename RenderMeshT>
const std::shared_ptr<MeshT>& DrawableGeneric<MeshT, RenderMeshT>::mesh() const
{
    return _mesh;
}

template <typename MeshT, typename RenderMeshT>
void DrawableGeneric<MeshT, RenderMeshT>::setMaterial(const base::MaterialAttributes& mat, uint32_t submeshIndex)
{
    assert_submesh(submeshIndex);
    
    _submeshAttributes[submeshIndex].material = mat;
}

template <typename MeshT, typename RenderMeshT>
void DrawableGeneric<MeshT, RenderMeshT>::setSubmeshTransform(const glm::mat4& mat, uint32_t submeshIndex)
{
    assert_submesh(submeshIndex);
    _submeshAttributes[submeshIndex].transform = mat;
}

template <typename MeshT, typename RenderMeshT>
const base::MaterialAttributes& DrawableGeneric<MeshT, RenderMeshT>::material(uint32_t index) const
{
    assert_submesh_r(index, _defaultMaterial);
    return _submeshAttributes[index].material;
}

template <typename MeshT, typename RenderMeshT>
base::MaterialAttributes& DrawableGeneric<MeshT, RenderMeshT>::material(uint32_t index)
{
    assert_submesh_r(index, _defaultMaterial);
    return _submeshAttributes[index].material;
}

template <typename MeshT, typename RenderMeshT>
glm::mat4 DrawableGeneric<MeshT, RenderMeshT>::submeshTransform(uint32_t index) const
{
    assert_submesh_r(index, _transform);
    return _transform * _submeshAttributes[index].transform;
}

template <typename MeshT, typename RenderMeshT>
void DrawableGeneric<MeshT, RenderMeshT>::load(render::Vulkan * vk)
{
    if (_mesh.get() == nullptr)
    {
        throw std::runtime_error("DrawableGeneric::load(): Error loading render data. Invalid mesh data set.");
    }
    
    if (isLoaded())
    {
        bg2e_log_warning << "DrawableGeneric::load(): this drawable is already loaded" << bg2e_log_end;
        return;
    }
    
    _vulkan = vk;
    
    _renderMesh = std::make_shared<RenderMeshT>(_vulkan);
    _renderMesh->setMeshData(_mesh.get());
    _renderMesh->build();
    
    for (auto & submesh : _submeshAttributes)
    {
        auto renderMat = new render::MaterialBase(_vulkan);
        renderMat->setUseTextureCache(true);
        renderMat->setMaterialAttributes(submesh.material);
        renderMat->updateTextures();
        _materials.push_back(std::shared_ptr<render::MaterialBase>(renderMat));
    }
}

template <typename MeshT, typename RenderMeshT>
void DrawableGeneric<MeshT, RenderMeshT>::updateMaterials()
{
    for (auto & mat : _materials)
    {
        mat->updateTextures();
    }
}

template <typename MeshT, typename RenderMeshT>
std::shared_ptr<RenderMeshT> DrawableGeneric<MeshT, RenderMeshT>::renderMesh()
{
    if (_renderMesh.get() == nullptr)
    {
        throw std::runtime_error("DrawableGeneric::renderMesh(): No render mesh data found");
    }
    return _renderMesh;
}

template <typename MeshT, typename RenderMeshT>
const std::shared_ptr<render::MaterialBase>& DrawableGeneric<MeshT, RenderMeshT>::renderMaterial(uint32_t submeshIndex)
{
    if (submeshIndex >= _materials.size())
    {
        throw std::runtime_error("DrawableGeneric::renderMaterial(): submesh index out of bounds");
    }
    return _materials[submeshIndex];
}

template <typename MeshT, typename RenderMeshT>
void DrawableGeneric<MeshT, RenderMeshT>::drawSubmesh(
    VkCommandBuffer cmd,
    VkPipelineLayout layout,
    uint32_t submesh,
    DrawSubmeshFunction cb,
    VkPipelineBindPoint bp
) {
    auto mat = renderMaterial(submesh).get();
    auto trx = submeshTransform(submesh);
    
    auto descriptorSets = cb(mat, trx);
    
    _renderMesh->drawSubmesh(cmd, layout, descriptorSets, submesh, bp);
}

template <typename MeshT, typename RenderMeshT>
void DrawableGeneric<MeshT, RenderMeshT>::draw(
    VkCommandBuffer cmd,
    VkPipelineLayout layout,
    DrawFunction cb,
    VkPipelineBindPoint bp
) {
    for (auto i = 0; i < _materials.size(); ++i)
    {
        auto mat = _materials[i].get();
        auto trx = _transform * _submeshAttributes[i].transform;
        
        auto descriptorSets = cb(mat, trx, i);
        
        _renderMesh->drawSubmesh(cmd, layout, descriptorSets, i, bp);
        
    }
}


template void DrawableGeneric<geo::MeshP, render::vulkan::geo::MeshP>::setMesh(geo::MeshP* mesh);
template void DrawableGeneric<geo::MeshP, render::vulkan::geo::MeshP>::setMesh(std::shared_ptr<geo::MeshP> mesh);
template const std::shared_ptr<geo::MeshP>& DrawableGeneric<geo::MeshP, render::vulkan::geo::MeshP>::mesh() const;
template void DrawableGeneric<geo::MeshP, render::vulkan::geo::MeshP>::setMaterial(const base::MaterialAttributes& mat, uint32_t submeshIndex);
template void DrawableGeneric<geo::MeshP, render::vulkan::geo::MeshP>::setSubmeshTransform(const glm::mat4& mat, uint32_t submeshIndex);
template const base::MaterialAttributes& DrawableGeneric<geo::MeshP, render::vulkan::geo::MeshP>::material(uint32_t index) const;
template base::MaterialAttributes& DrawableGeneric<geo::MeshP, render::vulkan::geo::MeshP>::material(uint32_t index);
template glm::mat4 DrawableGeneric<geo::MeshP, render::vulkan::geo::MeshP>::submeshTransform(uint32_t index) const;
template void DrawableGeneric<geo::MeshP, render::vulkan::geo::MeshP>::load(render::Vulkan * vk);
template void DrawableGeneric<geo::MeshP, render::vulkan::geo::MeshP>::updateMaterials();
template std::shared_ptr<render::vulkan::geo::MeshP> DrawableGeneric<geo::MeshP, render::vulkan::geo::MeshP>::renderMesh();
template const std::shared_ptr<render::MaterialBase>& DrawableGeneric<geo::MeshP, render::vulkan::geo::MeshP>::renderMaterial(uint32_t submeshIndex);
template void DrawableGeneric<geo::MeshP, render::vulkan::geo::MeshP>::drawSubmesh(VkCommandBuffer cmd, VkPipelineLayout layout, uint32_t submesh, std::function<std::vector<VkDescriptorSet>(render::MaterialBase *, const glm::mat4&)> cb, VkPipelineBindPoint bp);
template void DrawableGeneric<geo::MeshP, render::vulkan::geo::MeshP>::draw(VkCommandBuffer cmd, VkPipelineLayout layout, std::function<std::vector<VkDescriptorSet>(render::MaterialBase *, const glm::mat4&, uint32_t submIndex)> cb, VkPipelineBindPoint bp);

template void DrawableGeneric<geo::MeshPN, render::vulkan::geo::MeshPN>::setMesh(geo::MeshPN* mesh);
template void DrawableGeneric<geo::MeshPN, render::vulkan::geo::MeshPN>::setMesh(std::shared_ptr<geo::MeshPN> mesh);
template const std::shared_ptr<geo::MeshPN>& DrawableGeneric<geo::MeshPN, render::vulkan::geo::MeshPN>::mesh() const;
template void DrawableGeneric<geo::MeshPN, render::vulkan::geo::MeshPN>::setMaterial(const base::MaterialAttributes& mat, uint32_t submeshIndex);
template void DrawableGeneric<geo::MeshPN, render::vulkan::geo::MeshPN>::setSubmeshTransform(const glm::mat4& mat, uint32_t submeshIndex);
template const base::MaterialAttributes& DrawableGeneric<geo::MeshPN, render::vulkan::geo::MeshPN>::material(uint32_t index) const;
template base::MaterialAttributes& DrawableGeneric<geo::MeshPN, render::vulkan::geo::MeshPN>::material(uint32_t index);
template glm::mat4 DrawableGeneric<geo::MeshPN, render::vulkan::geo::MeshPN>::submeshTransform(uint32_t index) const;
template void DrawableGeneric<geo::MeshPN, render::vulkan::geo::MeshPN>::load(render::Vulkan * vk);
template void DrawableGeneric<geo::MeshPN, render::vulkan::geo::MeshPN>::updateMaterials();
template std::shared_ptr<render::vulkan::geo::MeshPN> DrawableGeneric<geo::MeshPN, render::vulkan::geo::MeshPN>::renderMesh();
template const std::shared_ptr<render::MaterialBase>& DrawableGeneric<geo::MeshPN, render::vulkan::geo::MeshPN>::renderMaterial(uint32_t submeshIndex);
template void DrawableGeneric<geo::MeshPN, render::vulkan::geo::MeshPN>::drawSubmesh(VkCommandBuffer cmd, VkPipelineLayout layout, uint32_t submesh, std::function<std::vector<VkDescriptorSet>(render::MaterialBase *, const glm::mat4&)> cb, VkPipelineBindPoint bp);
template void DrawableGeneric<geo::MeshPN, render::vulkan::geo::MeshPN>::draw(VkCommandBuffer cmd, VkPipelineLayout layout, std::function<std::vector<VkDescriptorSet>(render::MaterialBase *, const glm::mat4&, uint32_t submIndex)> cb, VkPipelineBindPoint bp);

template void DrawableGeneric<geo::MeshPC, render::vulkan::geo::MeshPC>::setMesh(geo::MeshPC* mesh);
template void DrawableGeneric<geo::MeshPC, render::vulkan::geo::MeshPC>::setMesh(std::shared_ptr<geo::MeshPC> mesh);
template const std::shared_ptr<geo::MeshPC>& DrawableGeneric<geo::MeshPC, render::vulkan::geo::MeshPC>::mesh() const;
template void DrawableGeneric<geo::MeshPC, render::vulkan::geo::MeshPC>::setMaterial(const base::MaterialAttributes& mat, uint32_t submeshIndex);
template void DrawableGeneric<geo::MeshPC, render::vulkan::geo::MeshPC>::setSubmeshTransform(const glm::mat4& mat, uint32_t submeshIndex);
template const base::MaterialAttributes& DrawableGeneric<geo::MeshPC, render::vulkan::geo::MeshPC>::material(uint32_t index) const;
template base::MaterialAttributes& DrawableGeneric<geo::MeshPC, render::vulkan::geo::MeshPC>::material(uint32_t index);
template glm::mat4 DrawableGeneric<geo::MeshPC, render::vulkan::geo::MeshPC>::submeshTransform(uint32_t index) const;
template void DrawableGeneric<geo::MeshPC, render::vulkan::geo::MeshPC>::load(render::Vulkan * vk);
template void DrawableGeneric<geo::MeshPC, render::vulkan::geo::MeshPC>::updateMaterials();
template std::shared_ptr<render::vulkan::geo::MeshPC> DrawableGeneric<geo::MeshPC, render::vulkan::geo::MeshPC>::renderMesh();
template const std::shared_ptr<render::MaterialBase>& DrawableGeneric<geo::MeshPC, render::vulkan::geo::MeshPC>::renderMaterial(uint32_t submeshIndex);
template void DrawableGeneric<geo::MeshPC, render::vulkan::geo::MeshPC>::drawSubmesh(VkCommandBuffer cmd, VkPipelineLayout layout, uint32_t submesh, std::function<std::vector<VkDescriptorSet>(render::MaterialBase *, const glm::mat4&)> cb, VkPipelineBindPoint bp);
template void DrawableGeneric<geo::MeshPC, render::vulkan::geo::MeshPC>::draw(VkCommandBuffer cmd, VkPipelineLayout layout, std::function<std::vector<VkDescriptorSet>(render::MaterialBase *, const glm::mat4&, uint32_t submIndex)> cb, VkPipelineBindPoint bp);

template void DrawableGeneric<geo::MeshPU, render::vulkan::geo::MeshPU>::setMesh(geo::MeshPU* mesh);
template void DrawableGeneric<geo::MeshPU, render::vulkan::geo::MeshPU>::setMesh(std::shared_ptr<geo::MeshPU> mesh);
template const std::shared_ptr<geo::MeshPU>& DrawableGeneric<geo::MeshPU, render::vulkan::geo::MeshPU>::mesh() const;
template void DrawableGeneric<geo::MeshPU, render::vulkan::geo::MeshPU>::setMaterial(const base::MaterialAttributes& mat, uint32_t submeshIndex);
template void DrawableGeneric<geo::MeshPU, render::vulkan::geo::MeshPU>::setSubmeshTransform(const glm::mat4& mat, uint32_t submeshIndex);
template const base::MaterialAttributes& DrawableGeneric<geo::MeshPU, render::vulkan::geo::MeshPU>::material(uint32_t index) const;
template base::MaterialAttributes& DrawableGeneric<geo::MeshPU, render::vulkan::geo::MeshPU>::material(uint32_t index);
template glm::mat4 DrawableGeneric<geo::MeshPU, render::vulkan::geo::MeshPU>::submeshTransform(uint32_t index) const;
template void DrawableGeneric<geo::MeshPU, render::vulkan::geo::MeshPU>::load(render::Vulkan * vk);
template void DrawableGeneric<geo::MeshPU, render::vulkan::geo::MeshPU>::updateMaterials();
template std::shared_ptr<render::vulkan::geo::MeshPU> DrawableGeneric<geo::MeshPU, render::vulkan::geo::MeshPU>::renderMesh();
template const std::shared_ptr<render::MaterialBase>& DrawableGeneric<geo::MeshPU, render::vulkan::geo::MeshPU>::renderMaterial(uint32_t submeshIndex);
template void DrawableGeneric<geo::MeshPU, render::vulkan::geo::MeshPU>::drawSubmesh(VkCommandBuffer cmd, VkPipelineLayout layout, uint32_t submesh, std::function<std::vector<VkDescriptorSet>(render::MaterialBase *, const glm::mat4&)> cb, VkPipelineBindPoint bp);
template void DrawableGeneric<geo::MeshPU, render::vulkan::geo::MeshPU>::draw(VkCommandBuffer cmd, VkPipelineLayout layout, std::function<std::vector<VkDescriptorSet>(render::MaterialBase *, const glm::mat4&, uint32_t submIndex)> cb, VkPipelineBindPoint bp);

template void DrawableGeneric<geo::MeshPNU, render::vulkan::geo::MeshPNU>::setMesh(geo::MeshPNU* mesh);
template void DrawableGeneric<geo::MeshPNU, render::vulkan::geo::MeshPNU>::setMesh(std::shared_ptr<geo::MeshPNU> mesh);
template const std::shared_ptr<geo::MeshPNU>& DrawableGeneric<geo::MeshPNU, render::vulkan::geo::MeshPNU>::mesh() const;
template void DrawableGeneric<geo::MeshPNU, render::vulkan::geo::MeshPNU>::setMaterial(const base::MaterialAttributes& mat, uint32_t submeshIndex);
template void DrawableGeneric<geo::MeshPNU, render::vulkan::geo::MeshPNU>::setSubmeshTransform(const glm::mat4& mat, uint32_t submeshIndex);
template const base::MaterialAttributes& DrawableGeneric<geo::MeshPNU, render::vulkan::geo::MeshPNU>::material(uint32_t index) const;
template base::MaterialAttributes& DrawableGeneric<geo::MeshPNU, render::vulkan::geo::MeshPNU>::material(uint32_t index);
template glm::mat4 DrawableGeneric<geo::MeshPNU, render::vulkan::geo::MeshPNU>::submeshTransform(uint32_t index) const;
template void DrawableGeneric<geo::MeshPNU, render::vulkan::geo::MeshPNU>::load(render::Vulkan * vk);
template void DrawableGeneric<geo::MeshPNU, render::vulkan::geo::MeshPNU>::updateMaterials();
template std::shared_ptr<render::vulkan::geo::MeshPNU> DrawableGeneric<geo::MeshPNU, render::vulkan::geo::MeshPNU>::renderMesh();
template const std::shared_ptr<render::MaterialBase>& DrawableGeneric<geo::MeshPNU, render::vulkan::geo::MeshPNU>::renderMaterial(uint32_t submeshIndex);
template void DrawableGeneric<geo::MeshPNU, render::vulkan::geo::MeshPNU>::drawSubmesh(VkCommandBuffer cmd, VkPipelineLayout layout, uint32_t submesh, std::function<std::vector<VkDescriptorSet>(render::MaterialBase *, const glm::mat4&)> cb, VkPipelineBindPoint bp);
template void DrawableGeneric<geo::MeshPNU, render::vulkan::geo::MeshPNU>::draw(VkCommandBuffer cmd, VkPipelineLayout layout, std::function<std::vector<VkDescriptorSet>(render::MaterialBase *, const glm::mat4&, uint32_t submIndex)> cb, VkPipelineBindPoint bp);

template void DrawableGeneric<geo::MeshPNC, render::vulkan::geo::MeshPNC>::setMesh(geo::MeshPNC* mesh);
template void DrawableGeneric<geo::MeshPNC, render::vulkan::geo::MeshPNC>::setMesh(std::shared_ptr<geo::MeshPNC> mesh);
template const std::shared_ptr<geo::MeshPNC>& DrawableGeneric<geo::MeshPNC, render::vulkan::geo::MeshPNC>::mesh() const;
template void DrawableGeneric<geo::MeshPNC, render::vulkan::geo::MeshPNC>::setMaterial(const base::MaterialAttributes& mat, uint32_t submeshIndex);
template void DrawableGeneric<geo::MeshPNC, render::vulkan::geo::MeshPNC>::setSubmeshTransform(const glm::mat4& mat, uint32_t submeshIndex);
template const base::MaterialAttributes& DrawableGeneric<geo::MeshPNC, render::vulkan::geo::MeshPNC>::material(uint32_t index) const;
template base::MaterialAttributes& DrawableGeneric<geo::MeshPNC, render::vulkan::geo::MeshPNC>::material(uint32_t index);
template glm::mat4 DrawableGeneric<geo::MeshPNC, render::vulkan::geo::MeshPNC>::submeshTransform(uint32_t index) const;
template void DrawableGeneric<geo::MeshPNC, render::vulkan::geo::MeshPNC>::load(render::Vulkan * vk);
template void DrawableGeneric<geo::MeshPNC, render::vulkan::geo::MeshPNC>::updateMaterials();
template std::shared_ptr<render::vulkan::geo::MeshPNC> DrawableGeneric<geo::MeshPNC, render::vulkan::geo::MeshPNC>::renderMesh();
template const std::shared_ptr<render::MaterialBase>& DrawableGeneric<geo::MeshPNC, render::vulkan::geo::MeshPNC>::renderMaterial(uint32_t submeshIndex);
template void DrawableGeneric<geo::MeshPNC, render::vulkan::geo::MeshPNC>::drawSubmesh(VkCommandBuffer cmd, VkPipelineLayout layout, uint32_t submesh, std::function<std::vector<VkDescriptorSet>(render::MaterialBase *, const glm::mat4&)> cb, VkPipelineBindPoint bp);
template void DrawableGeneric<geo::MeshPNC, render::vulkan::geo::MeshPNC>::draw(VkCommandBuffer cmd, VkPipelineLayout layout, std::function<std::vector<VkDescriptorSet>(render::MaterialBase *, const glm::mat4&, uint32_t submIndex)> cb, VkPipelineBindPoint bp);

template void DrawableGeneric<geo::MeshPNUC, render::vulkan::geo::MeshPNUC>::setMesh(geo::MeshPNUC* mesh);
template void DrawableGeneric<geo::MeshPNUC, render::vulkan::geo::MeshPNUC>::setMesh(std::shared_ptr<geo::MeshPNUC> mesh);
template const std::shared_ptr<geo::MeshPNUC>& DrawableGeneric<geo::MeshPNUC, render::vulkan::geo::MeshPNUC>::mesh() const;
template void DrawableGeneric<geo::MeshPNUC, render::vulkan::geo::MeshPNUC>::setMaterial(const base::MaterialAttributes& mat, uint32_t submeshIndex);
template void DrawableGeneric<geo::MeshPNUC, render::vulkan::geo::MeshPNUC>::setSubmeshTransform(const glm::mat4& mat, uint32_t submeshIndex);
template const base::MaterialAttributes& DrawableGeneric<geo::MeshPNUC, render::vulkan::geo::MeshPNUC>::material(uint32_t index) const;
template base::MaterialAttributes& DrawableGeneric<geo::MeshPNUC, render::vulkan::geo::MeshPNUC>::material(uint32_t index);
template glm::mat4 DrawableGeneric<geo::MeshPNUC, render::vulkan::geo::MeshPNUC>::submeshTransform(uint32_t index) const;
template void DrawableGeneric<geo::MeshPNUC, render::vulkan::geo::MeshPNUC>::load(render::Vulkan * vk);
template void DrawableGeneric<geo::MeshPNUC, render::vulkan::geo::MeshPNUC>::updateMaterials();
template std::shared_ptr<render::vulkan::geo::MeshPNUC> DrawableGeneric<geo::MeshPNUC, render::vulkan::geo::MeshPNUC>::renderMesh();
template const std::shared_ptr<render::MaterialBase>& DrawableGeneric<geo::MeshPNUC, render::vulkan::geo::MeshPNUC>::renderMaterial(uint32_t submeshIndex);
template void DrawableGeneric<geo::MeshPNUC, render::vulkan::geo::MeshPNUC>::drawSubmesh(VkCommandBuffer cmd, VkPipelineLayout layout, uint32_t submesh, std::function<std::vector<VkDescriptorSet>(render::MaterialBase *, const glm::mat4&)> cb, VkPipelineBindPoint bp);
template void DrawableGeneric<geo::MeshPNUC, render::vulkan::geo::MeshPNUC>::draw(VkCommandBuffer cmd, VkPipelineLayout layout, std::function<std::vector<VkDescriptorSet>(render::MaterialBase *, const glm::mat4&, uint32_t submIndex)> cb, VkPipelineBindPoint bp);

template void DrawableGeneric<geo::MeshPNUT, render::vulkan::geo::MeshPNUT>::setMesh(geo::MeshPNUT* mesh);
template void DrawableGeneric<geo::MeshPNUT, render::vulkan::geo::MeshPNUT>::setMesh(std::shared_ptr<geo::MeshPNUT> mesh);
template const std::shared_ptr<geo::MeshPNUT>& DrawableGeneric<geo::MeshPNUT, render::vulkan::geo::MeshPNUT>::mesh() const;
template void DrawableGeneric<geo::MeshPNUT, render::vulkan::geo::MeshPNUT>::setMaterial(const base::MaterialAttributes& mat, uint32_t submeshIndex);
template void DrawableGeneric<geo::MeshPNUT, render::vulkan::geo::MeshPNUT>::setSubmeshTransform(const glm::mat4& mat, uint32_t submeshIndex);
template const base::MaterialAttributes& DrawableGeneric<geo::MeshPNUT, render::vulkan::geo::MeshPNUT>::material(uint32_t index) const;
template base::MaterialAttributes& DrawableGeneric<geo::MeshPNUT, render::vulkan::geo::MeshPNUT>::material(uint32_t index);
template glm::mat4 DrawableGeneric<geo::MeshPNUT, render::vulkan::geo::MeshPNUT>::submeshTransform(uint32_t index) const;
template void DrawableGeneric<geo::MeshPNUT, render::vulkan::geo::MeshPNUT>::load(render::Vulkan * vk);
template void DrawableGeneric<geo::MeshPNUT, render::vulkan::geo::MeshPNUT>::updateMaterials();
template std::shared_ptr<render::vulkan::geo::MeshPNUT> DrawableGeneric<geo::MeshPNUT, render::vulkan::geo::MeshPNUT>::renderMesh();
template const std::shared_ptr<render::MaterialBase>& DrawableGeneric<geo::MeshPNUT, render::vulkan::geo::MeshPNUT>::renderMaterial(uint32_t submeshIndex);
template void DrawableGeneric<geo::MeshPNUT, render::vulkan::geo::MeshPNUT>::drawSubmesh(VkCommandBuffer cmd, VkPipelineLayout layout, uint32_t submesh, std::function<std::vector<VkDescriptorSet>(render::MaterialBase *, const glm::mat4&)> cb, VkPipelineBindPoint bp);
template void DrawableGeneric<geo::MeshPNUT, render::vulkan::geo::MeshPNUT>::draw(VkCommandBuffer cmd, VkPipelineLayout layout, std::function<std::vector<VkDescriptorSet>(render::MaterialBase *, const glm::mat4&, uint32_t submIndex)> cb, VkPipelineBindPoint bp);

template void DrawableGeneric<geo::MeshPNUUT, render::vulkan::geo::MeshPNUUT>::setMesh(geo::MeshPNUUT* mesh);
template void DrawableGeneric<geo::MeshPNUUT, render::vulkan::geo::MeshPNUUT>::setMesh(std::shared_ptr<geo::MeshPNUUT> mesh);
template const std::shared_ptr<geo::MeshPNUUT>& DrawableGeneric<geo::MeshPNUUT, render::vulkan::geo::MeshPNUUT>::mesh() const;
template void DrawableGeneric<geo::MeshPNUUT, render::vulkan::geo::MeshPNUUT>::setMaterial(const base::MaterialAttributes& mat, uint32_t submeshIndex);
template void DrawableGeneric<geo::MeshPNUUT, render::vulkan::geo::MeshPNUUT>::setSubmeshTransform(const glm::mat4& mat, uint32_t submeshIndex);
template const base::MaterialAttributes& DrawableGeneric<geo::MeshPNUUT, render::vulkan::geo::MeshPNUUT>::material(uint32_t index) const;
template base::MaterialAttributes& DrawableGeneric<geo::MeshPNUUT, render::vulkan::geo::MeshPNUUT>::material(uint32_t index);
template glm::mat4 DrawableGeneric<geo::MeshPNUUT, render::vulkan::geo::MeshPNUUT>::submeshTransform(uint32_t index) const;
template void DrawableGeneric<geo::MeshPNUUT, render::vulkan::geo::MeshPNUUT>::load(render::Vulkan * vk);
template void DrawableGeneric<geo::MeshPNUUT, render::vulkan::geo::MeshPNUUT>::updateMaterials();
template std::shared_ptr<render::vulkan::geo::MeshPNUUT> DrawableGeneric<geo::MeshPNUUT, render::vulkan::geo::MeshPNUUT>::renderMesh();
template const std::shared_ptr<render::MaterialBase>& DrawableGeneric<geo::MeshPNUUT, render::vulkan::geo::MeshPNUUT>::renderMaterial(uint32_t submeshIndex);
template void DrawableGeneric<geo::MeshPNUUT, render::vulkan::geo::MeshPNUUT>::drawSubmesh(VkCommandBuffer cmd, VkPipelineLayout layout, uint32_t submesh, std::function<std::vector<VkDescriptorSet>(render::MaterialBase *, const glm::mat4&)> cb, VkPipelineBindPoint bp);
template void DrawableGeneric<geo::MeshPNUUT, render::vulkan::geo::MeshPNUUT>::draw(VkCommandBuffer cmd, VkPipelineLayout layout, std::function<std::vector<VkDescriptorSet>(render::MaterialBase *, const glm::mat4&, uint32_t submIndex)> cb, VkPipelineBindPoint bp);


}
