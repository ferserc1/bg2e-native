
#include <bg2e/geo/sphere.hpp>

#include <stdexcept>
#include <numbers>
#include <functional>

namespace bg2e::geo {

float epsilonRound(float v) {
    return std::abs(v) < std::numeric_limits<float>::epsilon() ? 0.0f : v;
}

void createBaseSphere(
    float radius, 
    uint32_t longitudes, 
    uint32_t latitudes,
    std::function<void(VertexPNUT)> vertexCallback,
	std::function<void(uint32_t)> indexCallback,
	std::function<void(uint32_t, uint32_t)> submeshCallback
) {
    float latAlpha = 0.0f;
    float longAlpha = 0.0f;
    float latDelta = std::numbers::pi_v<float> / float(latitudes);
    float longDelta = 2.0f * std::numbers::pi_v<float> / float(longitudes);
    float vDelta = 1.0f / float(latitudes);
    float uDelta = 1.0f / float(longitudes);
    for (uint32_t u = 0; u <= latitudes; ++u)
    {
        longAlpha = 0.0f;
        for (uint32_t v = 0; v <= longitudes; ++v)
        {
            float x = epsilonRound((std::sin(longAlpha) * std::sin(latAlpha)) * radius);
            float y = epsilonRound(std::cos(latAlpha) * radius);
            float z = epsilonRound((std::cos(longAlpha) * std::sin(latAlpha)) * radius);

            float nx = epsilonRound(std::sin(latAlpha) * std::sin(longAlpha));
            float ny = epsilonRound(std::cos(latAlpha));
            float nz = epsilonRound(std::sin(latAlpha) * std::cos(longAlpha));

            float uvx = v * uDelta;
            float uvy = u * vDelta;

            float tx = std::cos(longAlpha);
            float ty = 0.0f;
            float tz = -std::sin(longAlpha);

            vertexCallback({
                { x, y, z },
                { nx, ny, nz },
                { uvx, uvy },
                { tx, ty, tz }
            });
            longAlpha += longDelta;
        }
        latAlpha += latDelta;
    }

    uint32_t numIndices = 0;
    for (uint32_t u = 0; u < latitudes - 1; ++u)
    {
        for (uint32_t v = 0; v < longitudes; ++v)
        {
            uint32_t i0 = v + u * (longitudes + 1);
            uint32_t i1 = (longitudes + 1) * (1 + u) + v;
            uint32_t i2 = i1 + 1;
            

			indexCallback(i0);
			indexCallback(i1);
			indexCallback(i2);
			numIndices += 3;

            // The top and bottom caps are triangles, but the res
            // of the faces are quads. Here we add the second triangle
            // for each quad
            if (u > 0)
            {
                uint32_t i3 = i2;
                uint32_t i4 = i0 + 1;
                uint32_t i5 = i0;

				indexCallback(i3);
				indexCallback(i4);
				indexCallback(i5);
                numIndices += 3;
            }
        }
    }

    // Botom cap indices
    uint32_t u = latitudes - 1;
    for (uint32_t v = 0; v < longitudes; ++v)
    {
        uint32_t i0 = v + u * (longitudes + 1);
        uint32_t i1 = (longitudes + 1) * (1 + u) + v;
        uint32_t i2 = i0 + 1;

		indexCallback(i0);
		indexCallback(i1);
		indexCallback(i2);
        numIndices += 3;
    }

	submeshCallback(0, numIndices);
}

bg2e::geo::MeshP* createSphereP(float radius, uint32_t longitudes, uint32_t latitudes)
{
    auto sphere = new bg2e::geo::MeshP();
    createBaseSphere(radius, longitudes, latitudes,
        [&](VertexPNUT vertex) {
            sphere->vertices.push_back({
                vertex.position
            });
        },
        [&](uint32_t index) {
            sphere->indices.push_back(index);
        },
        [&](uint32_t firstIndex, uint32_t indexCount) {
            sphere->submeshes.push_back({ firstIndex, indexCount });
        }
    );
    return sphere;
}

bg2e::geo::MeshPC* createSpherePC(float radius, uint32_t longitudes, uint32_t latitudes)
{
    auto sphere = new bg2e::geo::MeshPC();
    createBaseSphere(radius, longitudes, latitudes,
        [&](VertexPNUT vertex) {
            sphere->vertices.push_back({
                vertex.position,
                { 1.0f, 1.0f, 1.0f, 1.0f }
            });
        },
        [&](uint32_t index) {
            sphere->indices.push_back(index);
        },
        [&](uint32_t firstIndex, uint32_t indexCount) {
            sphere->submeshes.push_back({ firstIndex, indexCount });
        }
    );
    return sphere;
}

bg2e::geo::MeshPN* createSpherePN(float radius, uint32_t longitudes, uint32_t latitudes)
{
    auto sphere = new bg2e::geo::MeshPN();
    createBaseSphere(radius, longitudes, latitudes,
        [&](VertexPNUT vertex) {
            sphere->vertices.push_back({
                vertex.position,
                vertex.normal
            });
        },
        [&](uint32_t index) {
            sphere->indices.push_back(index);
        },
        [&](uint32_t firstIndex, uint32_t indexCount) {
            sphere->submeshes.push_back({ firstIndex, indexCount });
        }
    );
    return sphere;
}

bg2e::geo::MeshPU* createSpherePU(float radius, uint32_t longitudes, uint32_t latitudes)
{
    auto sphere = new bg2e::geo::MeshPU();
    createBaseSphere(radius, longitudes, latitudes,
        [&](VertexPNUT vertex) {
            sphere->vertices.push_back({
                vertex.position,
                vertex.texCoord0
            });
        },
        [&](uint32_t index) {
            sphere->indices.push_back(index);
        },
        [&](uint32_t firstIndex, uint32_t indexCount) {
            sphere->submeshes.push_back({ firstIndex, indexCount });
        }
    );
    return sphere;
}

bg2e::geo::MeshPNU* createSpherePNU(float radius, uint32_t longitudes, uint32_t latitudes)
{
    auto sphere = new bg2e::geo::MeshPNU();
    createBaseSphere(radius, longitudes, latitudes,
        [&](VertexPNUT vertex) {
            sphere->vertices.push_back({
                vertex.position,
                vertex.normal,
                vertex.texCoord0
            });
        },
        [&](uint32_t index) {
            sphere->indices.push_back(index);
        },
        [&](uint32_t firstIndex, uint32_t indexCount) {
            sphere->submeshes.push_back({ firstIndex, indexCount });
        }
    );
    return sphere;
}

bg2e::geo::MeshPNC* createSpherePNC(float radius, uint32_t longitudes, uint32_t latitudes)
{
    auto sphere = new bg2e::geo::MeshPNC();
    createBaseSphere(radius, longitudes, latitudes,
        [&](VertexPNUT vertex) {
            sphere->vertices.push_back({
                vertex.position,
                vertex.normal,
				{ 1.0f, 1.0f, 1.0f, 1.0f }
            });
        },
        [&](uint32_t index) {
            sphere->indices.push_back(index);
        },
        [&](uint32_t firstIndex, uint32_t indexCount) {
            sphere->submeshes.push_back({ firstIndex, indexCount });
        }
    );
    return sphere;
}

bg2e::geo::MeshPNUC* createSpherePNUC(float radius, uint32_t longitudes, uint32_t latitudes)
{
    auto sphere = new bg2e::geo::MeshPNUC();
    createBaseSphere(radius, longitudes, latitudes,
        [&](VertexPNUT vertex) {
            sphere->vertices.push_back({
                vertex.position,
                vertex.normal,
                vertex.texCoord0,
                { 1.0f, 1.0f, 1.0f, 1.0f}
            });
        },
        [&](uint32_t index) {
            sphere->indices.push_back(index);
        },
        [&](uint32_t firstIndex, uint32_t indexCount) {
            sphere->submeshes.push_back({ firstIndex, indexCount });
        }
    );
    return sphere;
}

bg2e::geo::MeshPNUT* createSpherePNUT(float radius, uint32_t longitudes, uint32_t latitudes)
{
    auto sphere = new bg2e::geo::MeshPNUT();
    createBaseSphere(radius, longitudes, latitudes,
        [&](VertexPNUT vertex) {
			sphere->vertices.push_back(vertex);
        },
        [&](uint32_t index) {
			sphere->indices.push_back(index);
		},
		[&](uint32_t firstIndex, uint32_t indexCount) {
			sphere->submeshes.push_back({ firstIndex, indexCount });
		}
    );
	return sphere;
}

bg2e::geo::MeshPNUUT* createSpherePNUUT(float radius, uint32_t longitudes, uint32_t latitudes)
{
    auto sphere = new bg2e::geo::MeshPNUUT();
    createBaseSphere(radius, longitudes, latitudes,
        [&](VertexPNUT vertex) {
            sphere->vertices.push_back({
				vertex.position,
				vertex.normal,
				vertex.texCoord0,
                vertex.texCoord0,
				vertex.tangent
            });
        },
        [&](uint32_t index) {
            sphere->indices.push_back(index);
        },
        [&](uint32_t firstIndex, uint32_t indexCount) {
            sphere->submeshes.push_back({ firstIndex, indexCount });
        }
    );
    return sphere;
}

bg2e::geo::MeshPNUUT* createSphere(float radius, uint32_t longitudes, uint32_t latitudes)
{
    return createSpherePNUUT(radius, longitudes, latitudes);
}

}
