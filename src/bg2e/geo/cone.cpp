
#include <bg2e/geo/cone.hpp>

#include <stdexcept>
#include <numbers>
#include <functional>

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
