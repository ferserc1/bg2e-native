
#include <bg2e/geo/cone.hpp>

#include <stdexcept>
#include <numbers>
#include <functional>
#include <iostream>

namespace bg2e::geo {

void createConeBase(
    float radius,
    float height,
    float slices,
    bool invertFaces,
    std::function<void(VertexPNUT)> vertexCallback,
    std::function<void(uint32_t)> indexCallback,
    std::function<void(uint32_t, uint32_t)> submeshCallback
) {
    float halfHeight = height / 2.0f;
    float angleDelta = 2.0f * std::numbers::pi_v<float> / float(slices);
    
    float alpha = 0.0f;
    // Base
    for (uint32_t i = 0; i < slices; ++i)
    {
        float x = std::sin(-alpha) * radius;
        float y = -halfHeight;
        float z = std::cos(-alpha) * radius;
        
        float u = 0.5f + 0.5f * std::sin(-alpha);
        float v = 0.5f - 0.5f * std::cos(-alpha);
        
        vertexCallback({
            { x, y , z },
            { 0.0f, -1.0f, 0.0f },
            { u, v },
            { -1.0f, 0.0f, 0.0f }
        });
        
        alpha += angleDelta;
    }
    
    // Cone body
    alpha = 0.0f;
    float vDelta = 1.0f / float(slices);
    for (uint32_t i = 0; i < slices; ++i)
    {
        glm::vec3 A {
            std::sin(alpha) * radius,
            -halfHeight,
            std::cos(alpha) * radius
        };
        glm::vec3 B {
            0.0f,
            halfHeight,
            0.0f
        };
        
        auto T = glm::normalize(B - A);
                
        float v = 0.0f;
        float u = vDelta * i;
        
        float phi = std::atan2(T.x, T.z);
        float theta = std::atan2(std::sqrt(T.z * T.z + T.x * T.x), T.y);
        
        // TODO: Get the phi and theta angles for the normal
        float Nphi = std::numbers::pi_v<float> / 2.0f + phi;
        float Ntheta = std::numbers::pi_v<float> / 2.0f - theta;
        
        // TODO: Calculate the normal
        auto N = glm::vec3{
            std::cos(Nphi) * std::sin(Ntheta),
            std::sin(Nphi) * std::sin(Ntheta),
            std::cos(Ntheta)
        };
        
        // Vertex callback: bottom vertex
        vertexCallback({
            A,
            N,
            { u, v },
            T
        });
        
        v = 1.0f;
        
        // Vertex callback: top vertex
        vertexCallback({
            B,
            N,
            { u, v },
            T
        });
        
        std::cout << "T=" << T.x << "," << T.y << "," << T.z << "  N=" << N.x << "," << N.y << "," << N.z << std::endl;
        
        alpha += angleDelta;
    }
    
}

bg2e::geo::MeshP* createConeP(float radius, float height, uint32_t slices, bool invertFaces)
{
    auto result = new MeshP();
    createConeBase(
        radius,
        height,
        slices,
        invertFaces,
        [&](VertexPNUT vertex) {
            result->vertices.push_back({
                vertex.position
            });
        },
        [&](uint32_t index) {
            result->indices.push_back(index);
        },
        [&](uint32_t start, uint32_t count) {
            result->submeshes.push_back({ start, count });
        }
    );
    return result;
}

bg2e::geo::MeshPC* createConePC(float radius, float height, uint32_t slices, bool invertFaces)
{
    auto result = new MeshPC();
    createConeBase(
        radius,
        height,
        slices,
        invertFaces,
        [&](VertexPNUT vertex) {
            result->vertices.push_back({
                vertex.position,
                { 1.0f, 1.0f, 1.0f, 1.0f }
            });
        },
        [&](uint32_t index) {
            result->indices.push_back(index);
        },
        [&](uint32_t start, uint32_t count) {
            result->submeshes.push_back({ start, count });
        }
    );
    return result;
}

bg2e::geo::MeshPN* createConePN(float radius, float height, uint32_t slices, bool invertFaces)
{
    auto result = new MeshPN();
    createConeBase(
        radius,
        height,
        slices,
        invertFaces,
        [&](VertexPNUT vertex) {
            result->vertices.push_back({
                vertex.position,
                vertex.normal
            });
        },
        [&](uint32_t index) {
            result->indices.push_back(index);
        },
        [&](uint32_t start, uint32_t count) {
            result->submeshes.push_back({ start, count });
        }
    );
    return result;
}

bg2e::geo::MeshPU* createConePU(float radius, float height, uint32_t slices, bool invertFaces)
{
    auto result = new MeshPU();
    createConeBase(
        radius,
        height,
        slices,
        invertFaces,
        [&](VertexPNUT vertex) {
            result->vertices.push_back({
                vertex.position,
                vertex.texCoord0
            });
        },
        [&](uint32_t index) {
            result->indices.push_back(index);
        },
        [&](uint32_t start, uint32_t count) {
            result->submeshes.push_back({ start, count });
        }
    );
    return result;
}

bg2e::geo::MeshPNU* createConePNU(float radius, float height, uint32_t slices, bool invertFaces)
{
    auto result = new MeshPNU();
    createConeBase(
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
        },
        [&](uint32_t index) {
            result->indices.push_back(index);
        },
        [&](uint32_t start, uint32_t count) {
            result->submeshes.push_back({ start, count });
        }
    );
    return result;
}

bg2e::geo::MeshPNC* createConePNC(float radius, float height, uint32_t slices, bool invertFaces)
{
    auto result = new MeshPNC();
    createConeBase(
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
        },
        [&](uint32_t index) {
            result->indices.push_back(index);
        },
        [&](uint32_t start, uint32_t count) {
            result->submeshes.push_back({ start, count });
        }
    );
    return result;
}

bg2e::geo::MeshPNUC* createConePNUC(float radius, float height, uint32_t slices, bool invertFaces)
{
    auto result = new MeshPNUC();
    createConeBase(
        radius,
        height,
        slices,
        invertFaces,
        [&](VertexPNUT vertex) {
            result->vertices.push_back({
                vertex.position,
                vertex.normal,
                vertex.tangent,
                { 1.0f, 1.0f, 1.0f, 1.0f }
            });
        },
        [&](uint32_t index) {
            result->indices.push_back(index);
        },
        [&](uint32_t start, uint32_t count) {
            result->submeshes.push_back({ start, count });
        }
    );
    return result;
}

bg2e::geo::MeshPNUT* createConePNUT(float radius, float height, uint32_t slices, bool invertFaces)
{
    auto result = new MeshPNUT();
    createConeBase(
        radius,
        height,
        slices,
        invertFaces,
        [&](VertexPNUT vertex) {
            result->vertices.push_back(vertex);
        },
        [&](uint32_t index) {
            result->indices.push_back(index);
        },
        [&](uint32_t start, uint32_t count) {
            result->submeshes.push_back({ start, count });
        }
    );
    return result;
}

bg2e::geo::MeshPNUUT* createConePNUUT(float radius, float height, uint32_t slices, bool invertFaces)
{
    auto result = new MeshPNUUT();
    createConeBase(
        radius,
        height,
        slices,
        invertFaces,
        [&](VertexPNUT vertex) {
            result->vertices.push_back({
                vertex.position,
                vertex.normal,
                vertex.texCoord0,
                vertex.texCoord0,
                vertex.tangent
            });
        },
        [&](uint32_t index) {
            result->indices.push_back(index);
        },
        [&](uint32_t start, uint32_t count) {
            result->submeshes.push_back({ start, count });
        }
    );
    return result;
}


bg2e::geo::MeshPNUUT* createCone(float radius, float height, uint32_t slices, bool invertFaces)
{
    return createConePNUUT(radius, height, slices, invertFaces);
}

}
