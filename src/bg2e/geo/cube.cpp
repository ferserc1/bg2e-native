#include <bg2e/geo/cube.hpp>

#include <stdexcept>
#include <numbers>
#include <functional>

namespace bg2e::geo {

void createCubeBase(
    float width,
    float height,
    float depth,
    bool flipFaces,
    std::function<void(VertexPNUT)> vertexCallback,
    std::function<void(uint32_t)> indexCallback,
    std::function<void(uint32_t, uint32_t)> submeshCallback
)
{
    float halfWidth = width / 2.0f;
    float halfHeight = height / 2.0f;
    float halfDepth = depth / 2.0f;

    // Front face
    vertexCallback({
        { -halfWidth, halfHeight, halfDepth },
        { 0.0f, 0.0f, 1.0f },
        { 0.0f, 0.0f },
        { 1.0f, 0.0f, 0.0f }
    });
    vertexCallback({
        { halfWidth, halfHeight, halfDepth },
        { 0.0f, 0.0f, 1.0f },
        { 1.0f, 0.0f },
        { 1.0f, 0.0f, 0.0f }
    });
    vertexCallback({
        { halfWidth, -halfHeight, halfDepth },
        { 0.0f, 0.0f, 1.0f },
        { 1.0f, 1.0f },
        { 1.0f, 0.0f, 0.0f }
    });
    vertexCallback({
        { -halfWidth, -halfHeight, halfDepth },
        { 0.0f, 0.0f, 1.0f },
        { 0.0f, 1.0f },
        { 1.0f, 0.0f, 0.0f }
    });

    // Back face
    vertexCallback({
        { halfWidth, halfHeight, -halfDepth },
        { 0.0f, 0.0f, -1.0f },
        { 0.0f, 0.0f },
        { -1.0f, 0.0f, 0.0f }
    });
    vertexCallback({
        { -halfWidth, halfHeight, -halfDepth },
        { 0.0f, 0.0f, -1.0f },
        { 1.0f, 0.0f },
        { -1.0f, 0.0f, 0.0f }
    });
    vertexCallback({
        { -halfWidth, -halfHeight, -halfDepth },
        { 0.0f, 0.0f, -1.0f },
        { 1.0f, 1.0f },
        { -1.0f, 0.0f, 0.0f }
    });
    vertexCallback({
        { halfWidth, -halfHeight, -halfDepth },
        { 0.0f, 0.0f, -1.0f },
        { 0.0f, 1.0f },
        { -1.0f, 0.0f, 0.0f }
    });

    // Right face
    vertexCallback({
        { halfWidth, -halfHeight, halfDepth },
        { 1.0f, 0.0f, 0.0f },
        { 0.0f, 1.0f },
        { 0.0f, 0.0f, -1.0f }
    });
    vertexCallback({
        { halfWidth, halfHeight, halfDepth },
        { 1.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f },
        { 0.0f, 0.0f, -1.0f }
    });
    vertexCallback({
        { halfWidth, halfHeight, -halfDepth },
        { 1.0f, 0.0f, 0.0f },
        { 1.0f, 0.0f },
        { 0.0f, 0.0f, -1.0f }
    });
    vertexCallback({
        { halfWidth, -halfHeight, -halfDepth },
        { 1.0f, 0.0f,0.0f },
        { 1.0f, 1.0f },
        { 0.0f, 0.0f, -1.0f }
    });

    // Left face
    vertexCallback({
        { -halfWidth, -halfHeight, -halfDepth },
        { -1.0f, 0.0f, 0.0f },
        { 0.0f, 1.0f },
        { 0.0f, 0.0f, 1.0f }
    });
    vertexCallback({
        { -halfWidth, halfHeight, -halfDepth },
        { -1.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f },
        { 0.0f, 0.0f, 1.0f }
    });
    vertexCallback({
        { -halfWidth, halfHeight, halfDepth },
        { -1.0f, 0.0f, 0.0f },
        { 1.0f, 0.0f },
        { 0.0f, 0.0f, 1.0f }
    });
    vertexCallback({
        { -halfWidth, -halfHeight, halfDepth },
        { -1.0f, 0.0f,0.0f },
        { 1.0f, 1.0f },
        { 0.0f, 0.0f, 1.0f }
    });

    // Top face
    vertexCallback({
        { -halfWidth, -halfHeight, -halfDepth },
        { 0.0f, 1.0f, 0.0f },
        { 0.0f, 0.0f },
        { 1.0f, 0.0f, 0.0f }
    });
    vertexCallback({
        { -halfWidth, -halfHeight, halfDepth },
        { 0.0f, 1.0f, 0.0f },
        { 1.0f, 0.0f },
        { 1.0f, 0.0f, 0.0f }
    });
    vertexCallback({
        { halfWidth, -halfHeight, halfDepth },
        { 0.0f, 1.0f, 0.0f },
        { 1.0f, 1.0f },
        { 1.0f, 0.0f, 0.0f }
    });
    vertexCallback({
        { halfWidth, -halfHeight, -halfDepth },
        { 0.0f, 1.0f, 0.0f },
        { 0.0f, 1.0f },
        { 1.0f, 0.0f, 0.0f }
    });

    // Bottom face
    vertexCallback({
        { -halfWidth, halfHeight, halfDepth },
        { 0.0f, -1.0f, 0.0f },
        { 0.0f, 0.0f },
        { -1.0f, 0.0f, 0.0f }
    });
    vertexCallback({
        { -halfWidth, halfHeight, -halfDepth },
        { 0.0f, -1.0f, 0.0f },
        { 1.0f, 0.0f },
        { -1.0f, 0.0f, 0.0f }
    });
    vertexCallback({
        { halfWidth, halfHeight, -halfDepth },
        { 0.0f, -1.0f, 0.0f },
        { 1.0f, 1.0f },
        { -1.0f, 0.0f, 0.0f }
    });
    vertexCallback({
        { halfWidth, halfHeight, halfDepth },
        { 0.0f, -1.0f, 0.0f },
        { 0.0f, 1.0f },
        { -1.0f, 0.0f, 0.0f }
    });

    if (flipFaces)
    {
        indexCallback(2);
        indexCallback(1);
        indexCallback(0);
        indexCallback(0);
        indexCallback(3);
        indexCallback(2);

        indexCallback(6);
        indexCallback(5);
        indexCallback(4);
        indexCallback(4);
        indexCallback(7);
        indexCallback(6);

        indexCallback(10);
        indexCallback(9);
        indexCallback(8);
        indexCallback(8);
        indexCallback(11);
        indexCallback(10);

        indexCallback(14);
        indexCallback(13);
        indexCallback(12);
        indexCallback(12);
        indexCallback(15);
        indexCallback(14);

        indexCallback(18);
        indexCallback(17);
        indexCallback(16);
        indexCallback(16);
        indexCallback(19);
        indexCallback(18);

        indexCallback(22);
        indexCallback(21);
        indexCallback(20);
        indexCallback(20);
        indexCallback(23);
        indexCallback(22);
    }
    else
    {
        indexCallback(0);
        indexCallback(1);
        indexCallback(2);
        indexCallback(2);
        indexCallback(3);
        indexCallback(0);

        indexCallback(4);
        indexCallback(5);
        indexCallback(6);
        indexCallback(6);
        indexCallback(7);
        indexCallback(4);

        indexCallback(8);
        indexCallback(9);
        indexCallback(10);
        indexCallback(10);
        indexCallback(11);
        indexCallback(8);

        indexCallback(12);
        indexCallback(13);
        indexCallback(14);
        indexCallback(14);
        indexCallback(15);
        indexCallback(12);

        indexCallback(16);
        indexCallback(17);
        indexCallback(18);
        indexCallback(18);
        indexCallback(19);
        indexCallback(16);

        indexCallback(20);
        indexCallback(21);
        indexCallback(22);
        indexCallback(22);
        indexCallback(23);
        indexCallback(20);
    }


    submeshCallback(0, 6);
    submeshCallback(6, 6);
    submeshCallback(12, 6);
    submeshCallback(18, 6);
    submeshCallback(24, 6);
    submeshCallback(30, 6);
}

bg2e::geo::MeshP* createCubeP(float width, float height, float depth, bool flipFaces)
{
    auto result = new MeshP();
    createCubeBase(
        width,
        height,
        depth,
        flipFaces,
        [&](VertexPNUT vertex) {
            result->vertices.push_back({
                vertex.position
            });
        }, [&](uint32_t index) {
            result->indices.push_back(index);
        }, [&](uint32_t base, uint32_t count) {
            result->submeshes.push_back({ base, count });
        }   
    );
    return result;
}

bg2e::geo::MeshPC* createCubePC(float width, float height, float depth, bool flipFaces)
{
    auto result = new MeshPC();
    createCubeBase(
        width,
        height,
        depth,
        flipFaces,
        [&](VertexPNUT vertex) {
            result->vertices.push_back({
                vertex.position,
                { 1.0f, 1.0f, 1.0f, 1.0f }
            });
        }, [&](uint32_t index) {
            result->indices.push_back(index);
        }, [&](uint32_t base, uint32_t count) {
            result->submeshes.push_back({ base, count });
        }
    );
    return result;
}

bg2e::geo::MeshPN* createCubePN(float width, float height, float depth, bool flipFaces)
{
    auto result = new MeshPN();
    createCubeBase(
        width,
        height,
        depth,
        flipFaces,
        [&](VertexPNUT vertex) {
            result->vertices.push_back({
                vertex.position,
                vertex.normal
            });
        }, [&](uint32_t index) {
            result->indices.push_back(index);
        }, [&](uint32_t base, uint32_t count) {
            result->submeshes.push_back({ base, count });
        }
    );
    return result;
}

bg2e::geo::MeshPU* createCubePU(float width, float height, float depth, bool flipFaces)
{
    auto result = new MeshPU();
    createCubeBase(
        width,
        height,
        depth,
        flipFaces,
        [&](VertexPNUT vertex) {
            result->vertices.push_back({
                vertex.position,
                vertex.texCoord0
            });
        }, [&](uint32_t index) {
            result->indices.push_back(index);
        }, [&](uint32_t base, uint32_t count) {
            result->submeshes.push_back({ base, count });
        }
    );
    return result;
}

bg2e::geo::MeshPNU* createCubePNU(float width, float height, float depth, bool flipFaces)
{
    auto result = new MeshPNU();
    createCubeBase(
        width,
        height,
        depth,
        flipFaces,
        [&](VertexPNUT vertex) {
            result->vertices.push_back({
                vertex.position,
                vertex.normal,
                vertex.texCoord0
            });
        }, [&](uint32_t index) {
            result->indices.push_back(index);
        }, [&](uint32_t base, uint32_t count) {
            result->submeshes.push_back({ base, count });
        }
    );
    return result;
}

bg2e::geo::MeshPNC* createCubePNC(float width, float height, float depth, bool flipFaces)
{
    auto result = new MeshPNC();
    createCubeBase(
        width,
        height,
        depth,
        flipFaces,
        [&](VertexPNUT vertex) {
            result->vertices.push_back({
                vertex.position,
                vertex.normal,
                { 1.0f, 1.0f, 1.0f, 1.0f }
            });
        }, [&](uint32_t index) {
            result->indices.push_back(index);
        }, [&](uint32_t base, uint32_t count) {
            result->submeshes.push_back({ base, count });
        }
    );
    return result;
}

bg2e::geo::MeshPNUC* createCubePNUC(float width, float height, float depth, bool flipFaces)
{
    auto result = new MeshPNUC();
    createCubeBase(
        width,
        height,
        depth,
        flipFaces,
        [&](VertexPNUT vertex) {
            result->vertices.push_back({
                vertex.position,
                vertex.normal,
                vertex.texCoord0,
                { 1.0f, 1.0f, 1.0f, 1.0f }
            });
        }, [&](uint32_t index) {
            result->indices.push_back(index);
        }, [&](uint32_t base, uint32_t count) {
            result->submeshes.push_back({ base, count });
        }
    );
    return result;
}

bg2e::geo::MeshPNUT* createCubePNUT(float width, float height, float depth, bool flipFaces)
{
    auto result = new MeshPNUT();
    createCubeBase(
        width,
        height,
        depth,
        flipFaces,
        [&](VertexPNUT vertex) {
            result->vertices.push_back({
                vertex.position,
                vertex.normal,
                vertex.texCoord0,
                vertex.tangent
            });
        }, [&](uint32_t index) {
            result->indices.push_back(index);
        }, [&](uint32_t base, uint32_t count) {
            result->submeshes.push_back({ base, count });
        }
    );
    return result;
}

bg2e::geo::MeshPNUUT* createCubePNUUT(float width, float height, float depth, bool flipFaces)
{
    auto result = new MeshPNUUT();
    createCubeBase(
        width,
        height,
        depth,
        flipFaces,
        [&](VertexPNUT vertex) {
            result->vertices.push_back({
                vertex.position,
                vertex.normal,
                vertex.texCoord0,
                vertex.texCoord0,
				vertex.tangent
            });
        }, [&](uint32_t index) {
            result->indices.push_back(index);
        }, [&](uint32_t base, uint32_t count) {
            result->submeshes.push_back({ base, count });
        }
    );
    return result;
}

bg2e::geo::MeshPNUUT* createCube(float width, float height, float depth, bool flipFaces)
{
    return createCubePNUUT(width, height, depth, flipFaces);
}

}
