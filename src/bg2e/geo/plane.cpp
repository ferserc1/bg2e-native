
#include <bg2e/geo/plane.hpp>

#include <stdexcept>
#include <numbers>
#include <functional>

namespace bg2e::geo {

void createPlaneBase(
    float width,
    float depth,
    bool invertPlane,
    std::function<void(VertexPNUT)> vertexCallback,
    std::function<void(uint32_t)> indexCallback,
    std::function<void(uint32_t, uint32_t)> submeshCallback
) {
    float halfWidth = width / 2.0f;
    float halfDepth = depth / 2.0f;
    
    auto N = glm::vec3{ 0.0f, 1.0f, 0.0f };
    auto T = glm::vec3{ 0.0f, 0.0f,-1.0f };
    vertexCallback({
        { -halfWidth, 0.0f, halfDepth },
        N,
        { 0.0f, 0.0f },
        T
    });
    
    vertexCallback({
        { -halfWidth, 0.0f, -halfDepth },
        N,
        { 1.0f, 0.0f },
        T
    });
    
    vertexCallback({
        { halfWidth, 0.0f, -halfDepth },
        N,
        { 1.0f, 1.0f },
        T
    });
    
    vertexCallback({
        { halfWidth, 0.0f, halfDepth },
        N,
        { 0.0f, 1.0f },
        T
    });
    
    if (!invertPlane)
    {
        indexCallback(2);
        indexCallback(1);
        indexCallback(0);
        indexCallback(0);
        indexCallback(3);
        indexCallback(2);
    }
    else
    {
        indexCallback(0);
        indexCallback(1);
        indexCallback(2);
        indexCallback(2);
        indexCallback(3);
        indexCallback(0);
    }
    
    submeshCallback(0, 6);
}

bg2e::geo::MeshP* createPlaneP(float width, float depth, bool invertFace)
{
    auto result = new MeshP();
    createPlaneBase(
        width,
        depth,
        invertFace,
        [&](VertexPNUT vertex) {
            result->vertices.push_back({
                vertex.position
            });
        },
        [&](uint32_t index) {
            result->indices.push_back(index);
        },
        [&](uint32_t indexStart, uint32_t indexEnd) {
            result->submeshes.push_back({ indexStart, indexEnd });
        }
    );
    return result;
}

bg2e::geo::MeshPC* createPlanePC(float width, float depth, bool invertFace)
{
    auto result = new MeshPC();
    createPlaneBase(
        width,
        depth,
        invertFace,
        [&](VertexPNUT vertex) {
            result->vertices.push_back({
                vertex.position,
                { 1.0f, 1.0f, 1.0f, 1.0f }
            });
        },
        [&](uint32_t index) {
            result->indices.push_back(index);
        },
        [&](uint32_t indexStart, uint32_t indexEnd) {
            result->submeshes.push_back({ indexStart, indexEnd });
        }
    );
    return result;
}

bg2e::geo::MeshPN* createPlanePN(float width, float depth, bool invertFace)
{
    auto result = new MeshPN();
    createPlaneBase(
        width,
        depth,
        invertFace,
        [&](VertexPNUT vertex) {
            result->vertices.push_back({
                vertex.position,
                vertex.normal
            });
        },
        [&](uint32_t index) {
            result->indices.push_back(index);
        },
        [&](uint32_t indexStart, uint32_t indexEnd) {
            result->submeshes.push_back({ indexStart, indexEnd });
        }
    );
    return result;
}

bg2e::geo::MeshPU* createPlanePU(float width, float depth, bool invertFace)
{
    auto result = new MeshPU();
    createPlaneBase(
        width,
        depth,
        invertFace,
        [&](VertexPNUT vertex) {
            result->vertices.push_back({
                vertex.position,
                vertex.texCoord0
            });
        },
        [&](uint32_t index) {
            result->indices.push_back(index);
        },
        [&](uint32_t indexStart, uint32_t indexEnd) {
            result->submeshes.push_back({ indexStart, indexEnd });
        }
    );
    return result;
}

bg2e::geo::MeshPNU* createPlanePNU(float width, float depth, bool invertFace)
{
    auto result = new MeshPNU();
    createPlaneBase(
        width,
        depth,
        invertFace,
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
        [&](uint32_t indexStart, uint32_t indexEnd) {
            result->submeshes.push_back({ indexStart, indexEnd });
        }
    );
    return result;
}

bg2e::geo::MeshPNC* createPlanePNC(float width, float depth, bool invertFace)
{
    auto result = new MeshPNC();
    createPlaneBase(
        width,
        depth,
        invertFace,
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
        [&](uint32_t indexStart, uint32_t indexEnd) {
            result->submeshes.push_back({ indexStart, indexEnd });
        }
    );
    return result;
}

bg2e::geo::MeshPNUC* createPlanePNUC(float width, float depth, bool invertFace)
{
    auto result = new MeshPNUC();
    createPlaneBase(
        width,
        depth,
        invertFace,
        [&](VertexPNUT vertex) {
            result->vertices.push_back({
                vertex.position,
                vertex.normal,
                vertex.texCoord0,
                { 1.0f, 1.0f, 1.0f, 1.0f }
            });
        },
        [&](uint32_t index) {
            result->indices.push_back(index);
        },
        [&](uint32_t indexStart, uint32_t indexEnd) {
            result->submeshes.push_back({ indexStart, indexEnd });
        }
    );
    return result;
}

bg2e::geo::MeshPNUT* createPlanePNUT(float width, float depth, bool invertFace)
{
    auto result = new MeshPNUT();
    createPlaneBase(
        width,
        depth,
        invertFace,
        [&](VertexPNUT vertex) {
            result->vertices.push_back(vertex);
        },
        [&](uint32_t index) {
            result->indices.push_back(index);
        },
        [&](uint32_t indexStart, uint32_t indexEnd) {
            result->submeshes.push_back({ indexStart, indexEnd });
        }
    );
    return result;
}

bg2e::geo::MeshPNUUT* createPlanePNUUT(float width, float depth, bool invertFace)
{
    auto result = new MeshPNUUT();
    createPlaneBase(
        width,
        depth,
        invertFace,
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
        [&](uint32_t indexStart, uint32_t indexEnd) {
            result->submeshes.push_back({ indexStart, indexEnd });
        }
    );
    return result;
}


bg2e::geo::MeshPNUUT* createPlane(float width, float depth, bool invertFace)
{
    return createPlanePNUUT(width, depth, invertFace);
}

}
