
#include <bg2e/geo/cylinder.hpp>

#include <stdexcept>
#include <numbers>
#include <functional>
#include <iostream>

namespace bg2e::geo {

void createCylinderBase(
    float radius,
    float height,
    uint32_t slices,
    bool invertFaces,
    std::function<void(VertexPNUT vertex)> vertexCallback,
    std::function<void(uint32_t index)> indexCallback,
    std::function<void(uint32_t start, uint32_t count)> submeshCallback
) {
    float halfHeight = height / 2.0f;
    float angleDelta = 2 * std::numbers::pi_v<float> / float(slices);
    
    // Top cap
    float alpha = 0.0f;
    // Center point
    vertexCallback({
        { 0.0f, halfHeight, 0.0f },
        { 0.0f, 1.0f, 0.0f },
        { 0.5f, 0.5f },
        { 1.0f, 0.0f, 0.0f }
    });
    uint32_t topCenterIndex = 0;
    
    for (uint32_t i = 0; i < slices; ++i)
    {
        float x = std::sin(alpha) * radius;
        float y = halfHeight;
        float z = std::cos(alpha) * radius;
        
        float u = 0.5f + 0.5f * std::sin(alpha);
        float v = 0.5f - 0.5f * std::cos(alpha);
        
        vertexCallback({
            { x, y, z },
            { 0.0f, 1.0f, 0.0f },
            { u, v },
            { 1.0f, 0.0f, 0.0f }
        });
        
        alpha += angleDelta;
    }
 
    // Bottom cap
    uint32_t bottomCenterIndex = 1 + slices; // topCenter + number of slices
    vertexCallback({
        { 0.0f, -halfHeight, 0.0f },
        { 0.0f, -1.0f, 0.0f },
        { 0.5f, 0.5f },
        { -1.0f, 0.0f, 0.0f }
    });
    
    for (uint32_t i = 0; i < slices; ++i)
    {
        float x = std::sin(-alpha) * radius;
        float y = -halfHeight;
        float z = std::cos(-alpha) * radius;
        
        float u = 0.5f + 0.5f * std::sin(-alpha);
        float v = 0.5f - 0.5f * std::cos(-alpha);
        
        vertexCallback({
            { x, y, z },
            { 0.0f, 1.0f, 0.0f },
            { u, v },
            { 1.0f, 0.0f, 0.0f }
        });
        
        alpha += angleDelta;
    }
    
    // Cylinder body
    alpha = 0.0f;
    float vDelta = 1.0f / float(slices);
    for (uint32_t i = 0; i <= slices; ++i)
    {
        float x = std::sin(alpha) * radius;
        float y = halfHeight;
        float z = std::cos(alpha) * radius;
        
        float nx = std::sin(alpha);
        float ny = 0.0f;
        float nz = std::cos(alpha);
        
        float v = 0.0f;
        float u = vDelta * i;
        
        vertexCallback({
            { x, y, z },
            { nx, ny, nz },
            { u, v },
            { 0.0f, -1.0f, 0.0f }
        });
            
        y = -halfHeight;
        
        v = 1.0f;
        
        vertexCallback({
            { x, y, z },
            { nx, ny, nz },
            { u, v },
            { 0.0f, -1.0f, 0.0f }
        });
        alpha += angleDelta;
    }
    
    // Indexes
    uint32_t i = 0;
    // Top cap
    for (; i < slices - 1; ++i)
    {
        if (!invertFaces)
        {
            indexCallback(topCenterIndex);
            indexCallback(i + 1);
            indexCallback(i + 2);
        }
        else
        {
            indexCallback(i + 2);
            indexCallback(i + 1);
            indexCallback(topCenterIndex);
        }
    }
    if (!invertFaces)
    {
        indexCallback(topCenterIndex);
        indexCallback(i + 1);
        indexCallback(topCenterIndex + 1);
    }
    else
    {
        indexCallback(topCenterIndex + 1);
        indexCallback(i + 1);
        indexCallback(topCenterIndex);
    }
    ++i;
    
    // Bottom cap
    for (uint32_t j = 0; j < slices; ++j)
    {
        if (!invertFaces)
        {
            indexCallback(bottomCenterIndex);
            indexCallback(i + 1);
            indexCallback(i + 2);
        }
        else
        {
            indexCallback(i + 2);
            indexCallback(i + 1);
            indexCallback(bottomCenterIndex);
        }
        ++i;
    }
    if (!invertFaces)
    {
        indexCallback(bottomCenterIndex);
        indexCallback(i + 1);
        indexCallback(bottomCenterIndex + 1);
    }
    else
    {
        indexCallback(bottomCenterIndex + 1);
        indexCallback(i + 1);
        indexCallback(bottomCenterIndex);
    }
    i += 2;
    
    // cylinder body
    for (uint32_t j = 0; j < slices; ++j)
    {
        uint32_t i0 = i;
        uint32_t i1 = i + 1;
        uint32_t i2 = i + 3;
        
        uint32_t i3 = i + 3;
        uint32_t i4 = i + 2;
        uint32_t i5 = i;
        
        if (!invertFaces)
        {
            indexCallback(i0);
            indexCallback(i1);
            indexCallback(i2);
            
            indexCallback(i3);
            indexCallback(i4);
            indexCallback(i5);
        }
        else
        {
            indexCallback(i2);
            indexCallback(i1);
            indexCallback(i0);
            
            indexCallback(i5);
            indexCallback(i4);
            indexCallback(i3);
        }
        
        i += 2;
    }
    
    submeshCallback(0, i);
}

bg2e::geo::MeshP* createCylinderP(float radius, float height, uint32_t slices, bool invertFaces)
{
    auto result = new MeshP();
    createCylinderBase(
        radius,
        height,
        slices,
        invertFaces,
        [&](VertexPNUT vertex) {
            result->vertices.push_back({
                vertex.position
            });
        }, [&](uint32_t index) {
            result->indices.push_back(index);
        }, [&](uint32_t start, uint32_t count) {
            result->submeshes.push_back({ start, count });
        }
    );
    return result;
}

bg2e::geo::MeshPC* createCylinderPC(float radius, float height, uint32_t slices, bool invertFaces)
{
    auto result = new MeshPC();
    createCylinderBase(
        radius,
        height,
        slices,
        invertFaces,
        [&](VertexPNUT vertex) {
            result->vertices.push_back({
                vertex.position,
                { 1.0f, 1.0f, 1.0f, 1.0f }
            });
        }, [&](uint32_t index) {
            result->indices.push_back(index);
        }, [&](uint32_t start, uint32_t count) {
            result->submeshes.push_back({ start, count });
        }
    );
    return result;
}

bg2e::geo::MeshPN* createCylinderPN(float radius, float height, uint32_t slices, bool invertFaces)
{
    auto result = new MeshPN();
    createCylinderBase(
        radius,
        height,
        slices,
        invertFaces,
        [&](VertexPNUT vertex) {
            result->vertices.push_back({
                vertex.position,
                vertex.normal
            });
        }, [&](uint32_t index) {
            result->indices.push_back(index);
        }, [&](uint32_t start, uint32_t count) {
            result->submeshes.push_back({ start, count });
        }
    );
    return result;
}

bg2e::geo::MeshPU* createCylinderPU(float radius, float height, uint32_t slices, bool invertFaces)
{
    auto result = new MeshPU();
    createCylinderBase(
        radius,
        height,
        slices,
        invertFaces,
        [&](VertexPNUT vertex) {
            result->vertices.push_back({
                vertex.position,
                vertex.texCoord0
            });
        }, [&](uint32_t index) {
            result->indices.push_back(index);
        }, [&](uint32_t start, uint32_t count) {
            result->submeshes.push_back({ start, count });
        }
    );
    return result;
}

bg2e::geo::MeshPNU* createCylinderPNU(float radius, float height, uint32_t slices, bool invertFaces)
{
    auto result = new MeshPNU();
    createCylinderBase(
        radius,
        height,
        slices,
        invertFaces,
        [&](VertexPNUT vertex) {
            result->vertices.push_back({
                vertex.position,
                vertex.normal,
                vertex.texCoord0
            });
        }, [&](uint32_t index) {
            result->indices.push_back(index);
        }, [&](uint32_t start, uint32_t count) {
            result->submeshes.push_back({ start, count });
        }
    );
    return result;
}

bg2e::geo::MeshPNC* createCylinderPNC(float radius, float height, uint32_t slices, bool invertFaces)
{
    auto result = new MeshPNC();
    createCylinderBase(
        radius,
        height,
        slices,
        invertFaces,
        [&](VertexPNUT vertex) {
            result->vertices.push_back({
                vertex.position,
                vertex.normal,
                { 1.0f, 1.0f, 1.0f, 1.0f }
            });
        }, [&](uint32_t index) {
            result->indices.push_back(index);
        }, [&](uint32_t start, uint32_t count) {
            result->submeshes.push_back({ start, count });
        }
    );
    return result;
}

bg2e::geo::MeshPNUC* createCylinderPNUC(float radius, float height, uint32_t slices, bool invertFaces)
{
    auto result = new MeshPNUC();
    createCylinderBase(
        radius,
        height,
        slices,
        invertFaces,
        [&](VertexPNUT vertex) {
            result->vertices.push_back({
                vertex.position,
                vertex.normal,
                vertex.texCoord0,
                { 1.0f, 1.0f, 1.0f, 1.0f }
            });
        }, [&](uint32_t index) {
            result->indices.push_back(index);
        }, [&](uint32_t start, uint32_t count) {
            result->submeshes.push_back({ start, count });
        }
    );
    return result;
}

bg2e::geo::MeshPNUT* createCylinderPNUT(float radius, float height, uint32_t slices, bool invertFaces)
{
    auto result = new MeshPNUT();
    createCylinderBase(
        radius,
        height,
        slices,
        invertFaces,
        [&](VertexPNUT vertex) {
            result->vertices.push_back(vertex);
        }, [&](uint32_t index) {
            result->indices.push_back(index);
        }, [&](uint32_t start, uint32_t count) {
            result->submeshes.push_back({ start, count });
        }
    );
    return result;
}

bg2e::geo::MeshPNUUT* createCylinderPNUUT(float radius, float height, uint32_t slices, bool invertFaces)
{
    auto result = new MeshPNUUT();
    createCylinderBase(
        radius,
        height,
        slices,
        invertFaces,
        [&](VertexPNUT vertex) {
            result->vertices.push_back({
                vertex.position,
                vertex.normal,
                vertex.texCoord0,
                vertex.tangent
            });
        }, [&](uint32_t index) {
            result->indices.push_back(index);
        }, [&](uint32_t start, uint32_t count) {
            result->submeshes.push_back({ start, count });
        }
    );
    return result;;
}


bg2e::geo::MeshPNUUT* createCylinder(float radius, float height, uint32_t slices, bool invertFaces)
{
    return createCylinderPNUUT(radius, height, slices, invertFaces);
}

}
