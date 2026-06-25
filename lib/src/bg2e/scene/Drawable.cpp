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
std::shared_ptr<DrawableBase> DrawableGeneric<MeshT, RenderMeshT>::clone() const
{
    auto copy = std::make_shared<DrawableGeneric<MeshT, RenderMeshT>>();

    // Copy the DrawableBase state (whole-mesh transform and name).
    copy->_transform = _transform;
    copy->_name = _name;

    // Deep copy the per-submesh attributes (material, transform, name, group,
    // visibility). MaterialAttributes is a value type, so this duplicates the
    // material definition rather than sharing it.
    copy->_submeshAttributes = _submeshAttributes;
    copy->_rayTracingEnabled = _rayTracingEnabled;

    // Deep copy the CPU mesh data. MeshGeneric is a plain struct of vectors, so a
    // copy duplicates the vertex/index buffers instead of sharing them.
    if (_mesh)
    {
        copy->_mesh = std::make_shared<MeshT>(*_mesh);
    }

    // If the source owns GPU resources, build independent ones for the copy so the
    // two drawables never share a render mesh or materials.
    if (isLoaded() && _engine && copy->_mesh)
    {
        copy->load(_engine);
    }

    return copy;
}

template <typename MeshT, typename RenderMeshT>
void DrawableGeneric<MeshT, RenderMeshT>::applyModifier(geo::Modifier<MeshT> * mod)
{
    if (!isLoaded())
    {
        std::cout << "[WARN] DrawableGeneric::applyModifier(): could not apply modifier because the drawable is not loaded." << std::endl;
        return;
    }

    mod->setMesh(_mesh.get());
    mod->apply();
    reload();
}

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
void DrawableGeneric<MeshT, RenderMeshT>::setSubmeshName(const std::string & name, uint32_t submeshIndex)
{
    assert_submesh(submeshIndex);
    _submeshAttributes[submeshIndex].name = name;
}

template <typename MeshT, typename RenderMeshT>
void DrawableGeneric<MeshT, RenderMeshT>::setSubmeshGroupName(const std::string& groupName, uint32_t submeshIndex)
{
    assert_submesh(submeshIndex);
    _submeshAttributes[submeshIndex].groupName = groupName;
}

template <typename MeshT, typename RenderMeshT>
void DrawableGeneric<MeshT, RenderMeshT>::setSubmeshVisibility(bool visible, uint32_t submeshIndex)
{
    assert_submesh(submeshIndex);
    _submeshAttributes[submeshIndex].visible = visible;
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
std::string DrawableGeneric<MeshT, RenderMeshT>::submeshName(uint32_t index) const
{
    assert_submesh_r(index, _defaultString);
    return _submeshAttributes[index].name;
}

template <typename MeshT, typename RenderMeshT>
std::string DrawableGeneric<MeshT, RenderMeshT>::submeshGroupName(uint32_t index) const
{
    assert_submesh_r(index, _defaultString);
    return _submeshAttributes[index].groupName;
}

template <typename MeshT, typename RenderMeshT>
bool DrawableGeneric<MeshT, RenderMeshT>::submeshVisibility(uint32_t index) const
{
    assert_submesh_r(index, false);
    return _submeshAttributes[index].visible;
}

template <typename MeshT, typename RenderMeshT>
uint32_t DrawableGeneric<MeshT, RenderMeshT>::submeshesCount() const
{
    return static_cast<uint32_t>(_submeshAttributes.size());
}

template <typename MeshT, typename RenderMeshT>
bg2e::geo::Submesh DrawableGeneric<MeshT, RenderMeshT>::submeshData(uint32_t index) const
{
    assert_submesh_r(index, _mesh->submeshes[index]);
    return _mesh->submeshes[index];
}

template <typename MeshT, typename RenderMeshT>
void DrawableGeneric<MeshT, RenderMeshT>::iterateMaterials(std::function<void(base::MaterialAttributes&)> cb)
{
    for (size_t i = 0; i < _submeshAttributes.size(); ++i)
    {
        cb(_submeshAttributes[i].material);
    }
}

template <typename MeshT, typename RenderMeshT>
glm::mat4 DrawableGeneric<MeshT, RenderMeshT>::submeshTransform(uint32_t index) const
{
    assert_submesh_r(index, _transform);
    return _transform * _submeshAttributes[index].transform;
}

template <typename MeshT, typename RenderMeshT>
void DrawableGeneric<MeshT, RenderMeshT>::load(render::Engine * engine)
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
    
    _engine = engine;

    buildRenderMesh();
    buildMaterials();
    buildRayTracingMeshes();
}

template <typename MeshT, typename RenderMeshT>
void DrawableGeneric<MeshT, RenderMeshT>::load(render::Engine * engine, std::function<void()> onTextureLoaded)
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

    _engine = engine;

    buildRenderMesh();
    buildMaterials(onTextureLoaded);
    buildRayTracingMeshes();
}

template <typename MeshT, typename RenderMeshT>
void DrawableGeneric<MeshT, RenderMeshT>::loadRenderMesh()
{
    buildRenderMesh();
}

template <typename MeshT, typename RenderMeshT>
void DrawableGeneric<MeshT, RenderMeshT>::buildMaterials(std::function<void()> onTextureLoaded)
{
    _materials.clear();

    for (auto & submesh : _submeshAttributes)
    {
        auto renderMat = new render::MaterialBase(_engine);
        renderMat->setUseTextureCache(true);
        renderMat->setMaterialAttributes(submesh.material);
        renderMat->updateTextures(onTextureLoaded);
        _materials.push_back(std::shared_ptr<render::MaterialBase>(renderMat));
    }
}

template <typename MeshT, typename RenderMeshT>
void DrawableGeneric<MeshT, RenderMeshT>::reload()
{
    if (!_engine || !_renderMesh.get() || _mesh.get() == nullptr)
    {
        throw std::runtime_error("DrawableGeneric::reload(): The mesh is not loaded");
    }

    // TODO: Reload the submesh attributes only if the number of submeshes has changed
    //_submeshAttributes.clear();
    //_submeshAttributes.resize(_mesh->submeshes.size());

    buildRenderMesh();
    buildMaterials();
    buildRayTracingMeshes();
}

template <typename MeshT, typename RenderMeshT>
void DrawableGeneric<MeshT, RenderMeshT>::updateMaterials()
{
    for (size_t i = 0; i < _submeshAttributes.size(); ++i)
    {
        auto renderMat = _materials[i].get();
        auto submesh = _submeshAttributes[i];
        renderMat->setMaterialAttributes(submesh.material);
        renderMat->updateTextures();
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
const std::shared_ptr<render::MaterialBase>& DrawableGeneric<MeshT, RenderMeshT>::renderMaterial(uint32_t submeshIndex) const
{
    if (submeshIndex >= _materials.size())
    {
        throw std::runtime_error("DrawableGeneric::renderMaterial(): submesh index out of bounds");
    }
    return _materials[submeshIndex];
}

template <typename MeshT, typename RenderMeshT>
std::shared_ptr<render::MaterialBase>& DrawableGeneric<MeshT, RenderMeshT>::renderMaterial(uint32_t submeshIndex)
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
    if (submeshVisibility(submesh))
    {
        auto mat = renderMaterial(submesh).get();
        auto trx = submeshTransform(submesh);
        
        auto descriptorSets = cb(mat, trx);
        
        _renderMesh->drawSubmesh(cmd, layout, descriptorSets, submesh, bp);
    }
}

template <typename MeshT, typename RenderMeshT>
void DrawableGeneric<MeshT, RenderMeshT>::draw(
    VkCommandBuffer cmd,
    VkPipelineLayout layout,
    DrawFunction cb,
    VkPipelineBindPoint bp
) {
    for (size_t i = 0; i < _materials.size(); ++i)
    {
        if (submeshVisibility(static_cast<uint32_t>(i)))
        {
            auto mat = _materials[i].get();
            auto trx = _transform * _submeshAttributes[i].transform;
            
            auto descriptorSets = cb(mat, trx, static_cast<uint32_t>(i));
            
            _renderMesh->drawSubmesh(cmd, layout, descriptorSets, static_cast<uint32_t>(i), bp);
        }
    }
}

template <typename MeshT, typename RenderMeshT>
void DrawableGeneric<MeshT, RenderMeshT>::buildRenderMesh()
{
    if (_renderMesh.get() == nullptr)
    {
        _renderMesh = std::make_shared<RenderMeshT>(_engine);
    }
    else
    {
        _renderMesh->cleanup();
    }

    _renderMesh->setMeshData(_mesh.get());
    _renderMesh->build();
}

template <typename MeshT, typename RenderMeshT>
void DrawableGeneric<MeshT, RenderMeshT>::buildMaterials()
{
    // Cleare existing materials
    _materials.clear();

    for (auto & submesh : _submeshAttributes)
    {
        auto renderMat = new render::MaterialBase(_engine);
        renderMat->setUseTextureCache(true);
        renderMat->setMaterialAttributes(submesh.material);
        renderMat->updateTextures();
        _materials.push_back(std::shared_ptr<render::MaterialBase>(renderMat));
    }
}

template <typename MeshT, typename RenderMeshT>
void DrawableGeneric<MeshT, RenderMeshT>::buildRayTracingMeshes()
{
    // Defer destruction of existing BLAS objects instead of using waitIdle().
    // The shared_ptrs are captured by value in the closure; they will be
    // destroyed when the closure runs after numImages() frames.
    if (!_rayTracingMeshes.empty())
    {
        auto meshesToDestroy = std::move(_rayTracingMeshes);
        _engine->deferredExec([meshesToDestroy]() mutable {
            for (auto mesh : meshesToDestroy)
            {
                mesh->cleanup();
            }
            meshesToDestroy.clear();
        });
    }

    if (rayTracingEnabled())
    {
        using namespace bg2e::render::vulkan::rt;

        for (uint32_t i = 0; i < submeshesCount(); ++i)
        {
            const auto&  sm = submeshData(i);
            auto rtMesh = std::make_shared<RayTracingMeshGeneric<MeshT>>(
                _engine,
                _renderMesh.get(),
                sm.firstIndex,
                sm.indexCount
            );
            rtMesh->build();
            _rayTracingMeshes.push_back(std::move(rtMesh));
        }
    }
}

template class BG2E_API DrawableGeneric<bg2e::geo::MeshP, bg2e::render::vulkan::geo::MeshP>;
template class BG2E_API DrawableGeneric<bg2e::geo::MeshPN, bg2e::render::vulkan::geo::MeshPN>;
template class BG2E_API DrawableGeneric<bg2e::geo::MeshPC, bg2e::render::vulkan::geo::MeshPC>;
template class BG2E_API DrawableGeneric<bg2e::geo::MeshPU, bg2e::render::vulkan::geo::MeshPU>;
template class BG2E_API DrawableGeneric<bg2e::geo::MeshPNU, bg2e::render::vulkan::geo::MeshPNU>;
template class BG2E_API DrawableGeneric<bg2e::geo::MeshPNC, bg2e::render::vulkan::geo::MeshPNC>;
template class BG2E_API DrawableGeneric<bg2e::geo::MeshPNUC, bg2e::render::vulkan::geo::MeshPNUC>;
template class BG2E_API DrawableGeneric<bg2e::geo::MeshPNUT, bg2e::render::vulkan::geo::MeshPNUT>;
template class BG2E_API DrawableGeneric<bg2e::geo::MeshPNUUT, bg2e::render::vulkan::geo::MeshPNUUT>;

}
