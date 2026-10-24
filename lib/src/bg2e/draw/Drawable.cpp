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

#include <bg2e/draw/Drawable.hpp>
#include <bg2e/base/CpuProfile.hpp>

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>

namespace bg2e::draw {
void Drawable::validateMesh(const Mesh& mesh)
{
    if (mesh.vertices.empty() || mesh.indices.empty() || mesh.vertices.size() > UINT32_MAX || mesh.indices.size() > UINT32_MAX)
    {
        throw std::invalid_argument("Drawable requires nonempty 32-bit indexed geometry");
    }
    for (auto index : mesh.indices)
    {
        if (index >= mesh.vertices.size())
        {
            throw std::invalid_argument("Drawable vertex index is out of range");
        }
    }
    for (const auto& submesh : mesh.submeshes)
    {
        if (submesh.firstIndex > mesh.indices.size() || submesh.indexCount > mesh.indices.size() - submesh.firstIndex ||
            submesh.indexCount == 0 || submesh.indexCount % 3 != 0)
        {
            throw std::invalid_argument("Drawable requires valid triangle submesh ranges");
        }
    }
}

void Drawable::setMesh(std::shared_ptr<Mesh> mesh)
{
    if (!mesh)
    {
        throw std::invalid_argument("Drawable mesh cannot be null");
    }
    validateMesh(*mesh);
    if (mesh->submeshes.empty())
    {
        if (mesh->indices.size() % 3 != 0)
        {
            throw std::invalid_argument("Drawable requires triangle indices");
        }
        mesh->submeshes.push_back({0, uint32_t(mesh->indices.size())});
    }
    if (_engine)
    {
        {
            base::CpuProfileScope profile("drawable.resourceSync.setMesh");
            base::CpuProfileScope::count("drawable.resourceSyncRequests.setMesh", 1, "requests");
            _engine->device()->waitIdleForResourceUpdate();
        }
    }
    _mesh = std::move(mesh);
    _attributes.resize(_mesh->submeshes.size());
    if (_engine)
    {
        reload();
    }
}

void Drawable::setName(const std::string& name)
{
    _name = name;
    std::replace(_name.begin(), _name.end(), '.', '_');
}

void Drawable::setMaterial(const base::MaterialAttributes& value, uint32_t index)
{
    material(index) = cloneMaterialAttributes(value);
}

base::MaterialAttributes& Drawable::material(uint32_t index)
{
    auto& attributes = _attributes.at(index);
    return index < _materials.size() && _materials[index] ? _materials[index]->materialAttributes() : attributes.material;
}

const base::MaterialAttributes& Drawable::material(uint32_t index) const
{
    const auto& attributes = _attributes.at(index);
    return index < _materials.size() && _materials[index] ? _materials[index]->materialAttributes() : attributes.material;
}

base::MaterialAttributes& Drawable::materialDefinition(uint32_t index)
{
    return _attributes.at(index).material;
}

const base::MaterialAttributes& Drawable::materialDefinition(uint32_t index) const
{
    return _attributes.at(index).material;
}

void Drawable::iterateMaterials(std::function<void(base::MaterialAttributes&)> operation)
{
    for (uint32_t i = 0; i < submeshesCount(); ++i)
    {
        operation(material(i));
    }
}

void Drawable::load(Engine* engine, std::function<void()> onTextureLoaded)
{
    if (!engine || (_engine && _engine != engine))
    {
        throw std::invalid_argument("Drawable cannot change engine without synchronized cleanup");
    }
    if (!_mesh)
    {
        throw std::logic_error("Drawable has no mesh");
    }
    validateMesh(*_mesh);
    if (_mesh->submeshes.size() != _attributes.size())
    {
        throw std::invalid_argument("Use setMesh to change Drawable submesh topology");
    }
    {
        base::CpuProfileScope profile("drawable.resourceSync.load");
        base::CpuProfileScope::count("drawable.resourceSyncRequests.load", 1, "requests");
        engine->device()->waitIdleForResourceUpdate();
    }
    _engine = engine;
    buildDrawMesh();
    updateMaterials(std::move(onTextureLoaded));
    buildRayTracingMeshes();
}

void Drawable::buildDrawMesh()
{
    _loaded = false;
    validateMesh(*_mesh);
    auto mesh = std::make_shared<DrawMesh>();
    mesh->setMeshData(*_mesh);
    mesh->build(_engine->device());
    _tracing.clear(); // Acceleration structures borrow the old raster buffers.
    _drawMesh = std::move(mesh);
}

void Drawable::updateMaterials(std::function<void()> onTextureLoaded)
{
    if (!_engine)
    {
        throw std::logic_error("Drawable has no engine");
    }
    {
        base::CpuProfileScope profile("drawable.resourceSync.updateMaterials");
        base::CpuProfileScope::count("drawable.resourceSyncRequests.updateMaterials", 1, "requests");
        _engine->device()->waitIdleForResourceUpdate();
    }
    _loaded = false;
    _materials.resize(_attributes.size());
    for (size_t i = 0; i < _materials.size(); ++i)
    {
        if (!_materials[i])
        {
            _materials[i] = std::make_shared<MaterialBase>(_engine);
            _materials[i]->setMaterialAttributes(cloneMaterialAttributes(_attributes[i].material));
        }
        _materials[i]->updateTextures(onTextureLoaded);
    }
    _loaded = bool(_drawMesh);
}

bool Drawable::rayTracingEnabled() const
{
    return _rayTracingEnabled && _engine && _engine->device()->rayTracingCapabilities().accelerationStructure;
}

void Drawable::buildRayTracingMeshes()
{
    _loaded = false;
    std::vector<std::shared_ptr<gpu::RayTracingMesh>> tracing;
    if (rayTracingEnabled())
    {
        for (uint32_t i = 0; i < _drawMesh->submeshCount(); ++i)
        {
            tracing.push_back(_engine->device()->createRayTracingMesh(_drawMesh->rayTracingMeshDescription(i)));
        }
        _engine->device()->immediateSubmit([&](gpu::CommandBuffer* command)
        {
            for (const auto& geometry : tracing)
            {
                command->buildRayTracingMesh(geometry.get());
            }
        });
    }
    _tracing = std::move(tracing);
    _loaded = true;
}

void Drawable::reload()
{
    if (!_engine || !_mesh)
    {
        throw std::logic_error("Drawable must be loaded before reload");
    }
    if (_mesh->submeshes.size() != _attributes.size())
    {
        throw std::invalid_argument("Use setMesh to change Drawable submesh topology");
    }
    {
        base::CpuProfileScope profile("drawable.resourceSync.reload");
        base::CpuProfileScope::count("drawable.resourceSyncRequests.reload", 1, "requests");
        _engine->device()->waitIdleForResourceUpdate();
    }
    buildDrawMesh();
    updateMaterials();
    buildRayTracingMeshes();
}

void Drawable::applyModifier(geo::Modifier<Mesh>* modifier)
{
    if (!_mesh || !modifier)
    {
        throw std::invalid_argument("Drawable modifier requires a mesh");
    }
    if (_engine)
    {
        {
            base::CpuProfileScope profile("drawable.resourceSync.applyModifier");
            base::CpuProfileScope::count("drawable.resourceSyncRequests.applyModifier", 1, "requests");
            _engine->device()->waitIdleForResourceUpdate();
        }
    }
    modifier->setMesh(_mesh.get());
    modifier->apply();
    if (_engine)
    {
        reload();
    }
}

std::shared_ptr<Drawable> Drawable::clone() const
{
    if (_engine && !isLoaded())
    {
        throw std::logic_error("Cannot clone an incomplete Drawable realization; retry load or clean up first");
    }
    auto result = std::make_shared<Drawable>();
    result->_name = _name;
    result->_transform = _transform;
    result->_attributes = _attributes;
    result->_rayTracingEnabled = _rayTracingEnabled;
    if (_mesh)
    {
        result->_mesh = std::make_shared<Mesh>(*_mesh);
    }
    for (uint32_t i = 0; i < submeshesCount(); ++i)
    {
        result->_attributes[i].material = cloneMaterialAttributes(material(i));
    }
    if (isLoaded())
    {
        {
            base::CpuProfileScope profile("drawable.resourceSync.clone");
            base::CpuProfileScope::count("drawable.resourceSyncRequests.clone", 1, "requests");
            _engine->device()->waitIdleForResourceUpdate();
        }
        result->_engine = _engine;
        result->_drawMesh = std::make_shared<DrawMesh>();
        // Copy the actual uploaded geometry, even if CPU edits await reload.
        result->_drawMesh->setMeshData(_drawMesh->meshData());
        result->_drawMesh->build(_engine->device());
        for (const auto& material : _materials)
        {
            result->_materials.push_back(material->clone());
        }
        if (!_tracing.empty())
        {
            result->_rayTracingEnabled = true;
            result->buildRayTracingMeshes();
            result->_rayTracingEnabled = _rayTracingEnabled;
        }
        result->_loaded = true;
    }
    return result;
}

void Drawable::cleanup()
{
    if (_engine)
    {
        {
            base::CpuProfileScope profile("drawable.resourceSync.cleanup");
            base::CpuProfileScope::count("drawable.resourceSyncRequests.cleanup", 1, "requests");
            _engine->device()->waitIdleForResourceUpdate();
        }
    }
    for (size_t i = 0; i < _materials.size() && i < _attributes.size(); ++i)
    {
        if (_materials[i])
        {
            _attributes[i].material = _materials[i]->materialAttributes();
        }
    }
    _tracing.clear();
    _materials.clear();
    _drawMesh.reset();
    _loaded = false;
    _engine = nullptr;
}

void Drawable::drawSubmesh(const FrameContext& frame, uint32_t index, const DrawFunction& bind) const
{
    if (!isLoaded() || &frame.engine != _engine || !bind)
    {
        throw std::invalid_argument("Drawable draw requires its own loaded context and a binding callback");
    }
    if (!submeshVisibility(index) || !material(index).visible())
    {
        return;
    }
    frame.commandBuffer.bindVertexBuffer(0, _drawMesh->vertexBuffer(), 0);
    frame.commandBuffer.bindIndexBuffer(_drawMesh->indexBuffer(), 0);
    bind(*_materials.at(index), submeshTransform(index), index);
    _drawMesh->drawSubmesh(&frame.commandBuffer, index);
}

void Drawable::draw(const FrameContext& frame, const DrawFunction& bind) const
{
    for (uint32_t i = 0; i < submeshesCount(); ++i)
    {
        drawSubmesh(frame, i, bind);
    }
}
}
