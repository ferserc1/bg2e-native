#pragma once

#include <bg2e/draw/MaterialBase.hpp>
#include <bg2e/draw/FrameContext.hpp>
#include <bg2e/gpu/Mesh.hpp>
#include <bg2e/gpu/RayTracingMesh.hpp>
#include <bg2e/geo/modifiers.hpp>

namespace bg2e::draw {
// Standard engine mesh: positions, normals, two UV sets and tangents.
// Geometry/materials are owned here; per-frame bindings belong to render passes.
class BG2E_API Drawable {
  public:
    using Mesh = geo::MeshPNUUT;
    using DrawMesh = gpu::MeshPNUUT;
    struct SubmeshAttributes {
        base::MaterialAttributes material;
        glm::mat4 transform{1.0f};
        std::string name;
        std::string groupName;
        bool visible = true;
    };
    using DrawFunction = std::function<void(MaterialBase&, const glm::mat4&, uint32_t)>;
    Drawable() = default;
    Drawable(const Drawable&) = delete;
    Drawable& operator=(const Drawable&) = delete;
    std::shared_ptr<Drawable> clone() const;

    void setMesh(std::shared_ptr<Mesh> mesh);
    void setMesh(Mesh* mesh)
    {
        setMesh(std::shared_ptr<Mesh>(mesh));
    }
    const std::shared_ptr<Mesh>& mesh() const
    {
        return _mesh;
    }
    void applyModifier(geo::Modifier<Mesh>* modifier);
    void load(Engine* engine, std::function<void()> onTextureLoaded = nullptr);
    void reload();
    void updateMaterials(std::function<void()> onTextureLoaded = nullptr);
    void cleanup(); // Synchronous; stop frame/background producers first.
    bool isLoaded() const
    {
        return _loaded;
    }
    Engine* engine() const
    {
        return _engine;
    }
    std::shared_ptr<DrawMesh> drawMesh() const
    {
        return _drawMesh;
    }
    std::shared_ptr<MaterialBase> drawMaterial(uint32_t index) const
    {
        return _materials.at(index);
    }
    const std::vector<std::shared_ptr<MaterialBase>>& materials() const
    {
        return _materials;
    }

    void setMaterial(const base::MaterialAttributes& value, uint32_t index = 0);
    base::MaterialAttributes& material(uint32_t index = 0);
    const base::MaterialAttributes& material(uint32_t index = 0) const;
    void iterateMaterials(std::function<void(base::MaterialAttributes&)> operation);
    void setSubmeshTransform(const glm::mat4& value, uint32_t index = 0)
    {
        _attributes.at(index).transform = value;
    }
    void setSubmeshName(const std::string& value, uint32_t index = 0)
    {
        _attributes.at(index).name = value;
    }
    void setSubmeshGroupName(const std::string& value, uint32_t index = 0)
    {
        _attributes.at(index).groupName = value;
    }
    void setSubmeshVisibility(bool value, uint32_t index = 0)
    {
        _attributes.at(index).visible = value;
    }
    glm::mat4 localSubmeshTransform(uint32_t index = 0) const
    {
        return _attributes.at(index).transform;
    }
    glm::mat4 submeshTransform(uint32_t index = 0) const
    {
        return _transform * localSubmeshTransform(index);
    }
    const std::string& submeshName(uint32_t index = 0) const
    {
        return _attributes.at(index).name;
    }
    const std::string& submeshGroupName(uint32_t index = 0) const
    {
        return _attributes.at(index).groupName;
    }
    bool submeshVisibility(uint32_t index = 0) const
    {
        return _attributes.at(index).visible;
    }
    uint32_t submeshesCount() const
    {
        return uint32_t(_attributes.size());
    }
    geo::Submesh submeshData(uint32_t index = 0) const
    {
        return _mesh->submeshes.at(index);
    }
    void setTransform(const glm::mat4& transform)
    {
        _transform = transform;
    }
    const glm::mat4& transform() const
    {
        return _transform;
    }
    glm::mat4& transform()
    {
        return _transform;
    }
    void setName(const std::string& name);
    const std::string& name() const
    {
        return _name;
    }

    void setRayTracingEnabled(bool value)
    {
        _rayTracingEnabled = value;
    }
    bool rayTracingEnabled() const;
    const std::vector<std::shared_ptr<gpu::RayTracingMesh>>& rayTracingMeshes() const
    {
        return _tracing;
    }
    std::shared_ptr<gpu::RayTracingMesh> rayTracingMesh(uint32_t index) const
    {
        return _tracing.at(index);
    }
    void draw(const FrameContext& frame, const DrawFunction& bind) const;
    void drawSubmesh(const FrameContext& frame, uint32_t index, const DrawFunction& bind) const;

  private:
    static void validateMesh(const Mesh& mesh);
    void buildDrawMesh();
    void buildRayTracingMeshes();
    std::shared_ptr<Mesh> _mesh;
    std::vector<SubmeshAttributes> _attributes;
    glm::mat4 _transform{1.0f};
    std::string _name;
    Engine* _engine = nullptr;
    std::shared_ptr<DrawMesh> _drawMesh;
    std::vector<std::shared_ptr<MaterialBase>> _materials;
    std::vector<std::shared_ptr<gpu::RayTracingMesh>> _tracing;
    bool _rayTracingEnabled = true;
    bool _loaded = false;
};
}
