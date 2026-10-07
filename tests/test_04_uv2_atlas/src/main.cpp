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

// CPU fixtures for geo::GenerateUv2AtlasModifier. This test does not create
// a window or any GPU resource; it links the engine only for the geo module.

#include <bg2e/geo/GenerateUv2AtlasModifier.hpp>
#include <bg2e/geo/UvAtlasValidator.hpp>
#include <bg2e/geo/cube.hpp>

#include <cmath>
#include <cstring>
#include <iostream>
#include <memory>
#include <stdexcept>

using namespace bg2e;

static int g_failures = 0;

static void check(bool condition, const std::string & message)
{
    if (condition)
    {
        std::cout << "[PASS] " << message << std::endl;
    }
    else
    {
        std::cout << "[FAIL] " << message << std::endl;
        ++g_failures;
    }
}

// A quad made of two triangles, each in its own submesh, sharing the two
// diagonal vertices. UV1 is a plain [0,1] mapping of the quad corners.
static geo::Mesh makeTwoSubmeshQuad()
{
    geo::Mesh mesh;
    mesh.vertices = {
        { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f }, { 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
        { { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
        { { 1.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f }, { 1.0f, 1.0f }, { 1.0f, 0.0f, 0.0f } },
        { { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f }, { 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f } }
    };
    mesh.indices = { 0, 1, 2, 0, 2, 3 };
    mesh.submeshes = { { 0, 3 }, { 3, 3 } };
    return mesh;
}

// A disconnected grid of quads to force an unrepresentable packing at a tiny
// fixed resolution (multi-atlas output).
static geo::Mesh makeQuadGrid(uint32_t quadsPerSide)
{
    geo::Mesh mesh;
    const float step = 1.0f;
    for (uint32_t qy = 0; qy < quadsPerSide; ++qy)
    {
        for (uint32_t qx = 0; qx < quadsPerSide; ++qx)
        {
            const float x = qx * step * 2.0f;
            const float y = qy * step * 2.0f;
            const auto base = static_cast<uint32_t>(mesh.vertices.size());
            mesh.vertices.push_back({ { x, y, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f }, { 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f } });
            mesh.vertices.push_back({ { x + step, y, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f } });
            mesh.vertices.push_back({ { x + step, y + step, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f }, { 1.0f, 1.0f }, { 1.0f, 0.0f, 0.0f } });
            mesh.vertices.push_back({ { x, y + step, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f }, { 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f } });
            mesh.indices.insert(mesh.indices.end(), { base, base + 1, base + 2, base, base + 2, base + 3 });
            mesh.submeshes.push_back({ static_cast<uint32_t>(mesh.indices.size()) - 6u, 6u });
        }
    }
    return mesh;
}

// Bit-for-bit attribute match (position, normal, UV1, tangent) against any
// source vertex. Fixtures contain no NaNs, so operator== is exact here.
static bool matchesSourceVertex(const geo::VertexPNUUT & v, const geo::Mesh & source)
{
    for (const auto & s : source.vertices)
    {
        if (std::memcmp(&v.position, &s.position, sizeof(v.position)) == 0 &&
            std::memcmp(&v.normal, &s.normal, sizeof(v.normal)) == 0 &&
            std::memcmp(&v.texCoord0, &s.texCoord0, sizeof(v.texCoord0)) == 0 &&
            std::memcmp(&v.tangent, &s.tangent, sizeof(v.tangent)) == 0)
        {
            return true;
        }
    }
    return false;
}

static bool uv2InUnitRange(const geo::Mesh & mesh)
{
    for (const auto & v : mesh.vertices)
    {
        if (!std::isfinite(v.texCoord1.x) || !std::isfinite(v.texCoord1.y) ||
            v.texCoord1.x < 0.0f || v.texCoord1.x > 1.0f ||
            v.texCoord1.y < 0.0f || v.texCoord1.y > 1.0f)
        {
            return false;
        }
    }
    return true;
}

static void testTwoSubmeshesSharedVertices()
{
    std::cout << "--- two submeshes, shared geometric vertices ---" << std::endl;
    auto mesh = makeTwoSubmeshQuad();
    const auto source = mesh;

    geo::GenerateUv2AtlasModifier mod(&mesh, { .resolution = 256, .paddingPixels = 4 });
    bool thrown = false;
    try
    {
        mod.apply();
    }
    catch (const std::exception & e)
    {
        thrown = true;
        std::cout << "       unexpected exception: " << e.what() << std::endl;
    }
    check(!thrown, "apply() succeeds on a two-submesh quad");

    const auto & result = mod.result();
    check(result.width == 256 && result.height == 256, "single atlas at requested resolution");
    check(result.chartCount >= 1, "at least one chart generated");
    check(result.utilization > 0.0f && result.utilization <= 1.0f, "utilization in (0, 1]");

    check(mesh.indices.size() == source.indices.size(), "triangle count preserved");
    check(mesh.submeshes.size() == 2 &&
          mesh.submeshes[0].firstIndex == 0 && mesh.submeshes[0].indexCount == 3 &&
          mesh.submeshes[1].firstIndex == 3 && mesh.submeshes[1].indexCount == 3,
          "submesh order and ranges preserved");
    check(uv2InUnitRange(mesh), "UV2 coordinates finite and in [0,1]");

    bool attributesCopied = true;
    for (const auto & v : mesh.vertices)
    {
        attributesCopied = attributesCopied && matchesSourceVertex(v, source);
    }
    check(attributesCopied, "position/normal/tangent/UV1 copied bit-for-bit from source vertices");
}

static void testCubeRegeneratesWholeObject()
{
    std::cout << "--- six-submesh cube ---" << std::endl;
    std::unique_ptr<geo::Mesh> cube(geo::createCube(1.0f, 1.0f, 1.0f));
    const auto source = *cube;
    check(!geo::UvAtlasValidator::validate(*cube, 1).valid,
        "cube starts with overlapping UV2 face mappings");

    // The old UV2 channel must have no influence on the generated layout.
    for (auto & vertex : cube->vertices)
    {
        vertex.texCoord1 = { 999.0f, 999.0f };
    }

    geo::GenerateUv2AtlasModifier mod(cube.get(), { .resolution = 256, .paddingPixels = 4 });
    mod.apply();
    check(cube->submeshes.size() == source.submeshes.size(),
        "all six material submeshes preserved");
    bool sameRanges = cube->submeshes.size() == source.submeshes.size();
    for (size_t i = 0; i < source.submeshes.size() && i < cube->submeshes.size(); ++i)
    {
        sameRanges = sameRanges &&
            cube->submeshes[i].firstIndex == source.submeshes[i].firstIndex &&
            cube->submeshes[i].indexCount == source.submeshes[i].indexCount;
    }
    check(sameRanges, "cube submesh index ranges preserved");
    check(geo::UvAtlasValidator::validate(*cube, 1).valid,
        "all six faces occupy one nonoverlapping UV2 atlas");
    bool attributesCopied = true;
    for (const auto & vertex : cube->vertices)
    {
        attributesCopied = attributesCopied && matchesSourceVertex(vertex, source);
    }
    check(attributesCopied, "cube UV1 and other source attributes preserved");
}

static void testExistingUv2Replaced()
{
    std::cout << "--- existing UV2 replacement ---" << std::endl;
    auto mesh = makeTwoSubmeshQuad();
    for (auto & v : mesh.vertices)
    {
        v.texCoord1 = { 999.0f, 999.0f };
    }

    geo::GenerateUv2AtlasModifier mod(&mesh, { .resolution = 128, .paddingPixels = 2 });
    mod.apply();
    check(uv2InUnitRange(mesh), "invalid previous UV2 values replaced by a valid atlas");
}

static void testInvalidInput()
{
    std::cout << "--- invalid input leaves the mesh unchanged ---" << std::endl;

    auto expectThrowKeepsMesh = [](geo::Mesh mesh, const char * message)
    {
        const auto source = mesh;
        bool thrown = false;
        try
        {
            geo::GenerateUv2AtlasModifier mod(&mesh);
            mod.apply();
        }
        catch (const std::runtime_error &)
        {
            thrown = true;
        }
        check(thrown, message);
        if (thrown)
        {
            check(mesh.vertices.size() == source.vertices.size() &&
                  mesh.indices == source.indices &&
                  mesh.submeshes.size() == source.submeshes.size(),
                  "  mesh unchanged after failure");
        }
    };

    {
        auto mesh = makeTwoSubmeshQuad();
        mesh.indices.push_back(0);
        mesh.submeshes[1].indexCount += 1;
        expectThrowKeepsMesh(mesh, "index count not divisible by 3 rejected");
    }
    {
        auto mesh = makeTwoSubmeshQuad();
        mesh.indices[0] = 42;
        expectThrowKeepsMesh(mesh, "index out of range rejected");
    }
    {
        auto mesh = makeTwoSubmeshQuad();
        mesh.submeshes.clear();
        expectThrowKeepsMesh(mesh, "missing submeshes rejected");
    }
    {
        auto mesh = makeTwoSubmeshQuad();
        mesh.submeshes[0].firstIndex = 3;
        expectThrowKeepsMesh(mesh, "non-covering submesh ranges rejected");
    }
    {
        bool thrown = false;
        try
        {
            geo::GenerateUv2AtlasModifier mod(nullptr);
            mod.apply();
        }
        catch (const std::runtime_error &)
        {
            thrown = true;
        }
        check(thrown, "null mesh rejected");
    }
}

static void testPackingFailure()
{
    std::cout << "--- atlas packing failure ---" << std::endl;

    // A 20x20 grid of disconnected quads cannot fit a 16x16 atlas with
    // 4 texels of padding; xatlas must emit several atlases and the modifier
    // must reject the multi-atlas result.
    auto mesh = makeQuadGrid(20);
    const auto sourceVertexCount = mesh.vertices.size();
    bool thrown = false;
    try
    {
        geo::GenerateUv2AtlasModifier mod(&mesh, { .resolution = 16, .paddingPixels = 4 });
        mod.apply();
    }
    catch (const std::runtime_error &)
    {
        thrown = true;
    }
    check(thrown, "unrepresentable packing (multi-atlas) rejected");
    check(mesh.vertices.size() == sourceVertexCount, "mesh unchanged after packing failure");

    auto mesh2 = makeTwoSubmeshQuad();
    bool thrown2 = false;
    try
    {
        geo::GenerateUv2AtlasModifier mod(&mesh2, { .resolution = 0, .paddingPixels = 4 });
        mod.apply();
    }
    catch (const std::runtime_error &)
    {
        thrown2 = true;
    }
    check(thrown2, "zero resolution rejected");
}

// Two disjoint quads; uv1 controls whether the UV2 map overlaps.
static geo::Mesh makeDoubleQuad(bool overlappingUv2)
{
    geo::Mesh mesh;
    const auto uvFor = [overlappingUv2](uint32_t corner) {
        // Non-overlapping layout: first quad on the left half, second on the right
        const glm::vec2 unique[4] = { { 0.0f, 0.0f }, { 0.5f, 0.0f }, { 0.5f, 1.0f }, { 0.0f, 1.0f } };
        const glm::vec2 shifted[4] = { { 0.5f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.5f, 1.0f } };
        const glm::vec2 full[4] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };
        return overlappingUv2 ? full[corner % 4] : (corner < 4 ? unique[corner] : shifted[corner % 4]);
    };
    for (uint32_t q = 0; q < 2; ++q)
    {
        const float x = q * 2.0f;
        const auto base = static_cast<uint32_t>(mesh.vertices.size());
        for (uint32_t c = 0; c < 4; ++c)
        {
            const auto uv = uvFor(q * 4 + c);
            mesh.vertices.push_back({
                { x + (c == 1 || c == 2 ? 1.0f : 0.0f), (c >= 2 ? 1.0f : 0.0f), 0.0f },
                { 0.0f, 0.0f, 1.0f },
                uv,
                uv,
                { 1.0f, 0.0f, 0.0f }
            });
        }
        mesh.indices.insert(mesh.indices.end(), { base, base + 1, base + 2, base, base + 2, base + 3 });
        mesh.submeshes.push_back({ static_cast<uint32_t>(mesh.indices.size()) - 6u, 6u });
    }
    return mesh;
}

static void checkValidation(const geo::Mesh & mesh, uint32_t uvSet, bool expectValid,
    geo::UvAtlasError expectedError, const std::string & message)
{
    const auto result = geo::UvAtlasValidator::validate(mesh, uvSet);
    check(result.valid == expectValid, message);
    if (!expectValid)
    {
        check(result.error == expectedError,
            "  expected error category " + std::to_string(static_cast<int>(expectedError)) +
            ", got " + std::to_string(static_cast<int>(result.error)) + " (" + result.message + ")");
    }
    else
    {
        check(result.error == geo::UvAtlasError::None, "  valid result reports no error");
        check(result.triangleCount * 3 == mesh.indices.size(), "  all triangles were validated");
        check(result.mappedArea > 0.0f, "  mapped area is positive");
        check(result.coverage > 0.0f && result.coverage <= 1.0f, "  coverage within (0, 1]");
    }
}

static void testValidator()
{
    std::cout << "--- UvAtlasValidator ---" << std::endl;

    // A valid UV1 map is also a valid UV2 atlas when copied (no provenance inference)
    checkValidation(makeTwoSubmeshQuad(), 0, true, geo::UvAtlasError::None, "valid UV1 map accepted");
    checkValidation(makeTwoSubmeshQuad(), 1, true, geo::UvAtlasError::None,
        "copied UV1 that is already a valid unique atlas accepted as UV2");

    checkValidation(makeDoubleQuad(false), 1, true, geo::UvAtlasError::None,
        "non-overlapping two-quad UV2 atlas accepted");

    checkValidation(makeDoubleQuad(true), 1, false, geo::UvAtlasError::OverlappingTriangles,
        "overlapping copied-UV1 fallback rejected as UV2");
    checkValidation(makeDoubleQuad(true), 0, false, geo::UvAtlasError::OverlappingTriangles,
        "overlapping UV1 map rejected as UV1");

    {
        auto mesh = makeTwoSubmeshQuad();
        mesh.vertices[0].texCoord1 = { 0.25f, 0.25f };
        mesh.vertices[1].texCoord1 = { 0.25f, 0.25f };
        mesh.vertices[2].texCoord1 = { 0.25f, 0.25f };
        checkValidation(mesh, 1, false, geo::UvAtlasError::DegenerateTriangle,
            "zero-area mapped triangle rejected");
    }
    {
        auto mesh = makeTwoSubmeshQuad();
        mesh.vertices[0].texCoord1 = { 1.5f, 0.0f };
        checkValidation(mesh, 1, false, geo::UvAtlasError::CoordinateOutOfRange,
            "coordinate outside [0,1] rejected");
    }
    {
        auto mesh = makeTwoSubmeshQuad();
        mesh.vertices[0].texCoord1 = { std::nanf(""), 0.0f };
        checkValidation(mesh, 1, false, geo::UvAtlasError::NonFiniteCoordinate,
            "non-finite coordinate rejected");
    }
    {
        auto mesh = makeTwoSubmeshQuad();
        mesh.submeshes.clear();
        checkValidation(mesh, 1, false, geo::UvAtlasError::MissingSubmeshes,
            "missing submeshes rejected");
    }
    {
        auto mesh = makeTwoSubmeshQuad();
        mesh.submeshes[1].firstIndex = 0;
        checkValidation(mesh, 1, false, geo::UvAtlasError::OverlappingSubmeshRanges,
            "overlapping submesh ranges rejected");
    }
    {
        auto mesh = makeTwoSubmeshQuad();
        mesh.submeshes[0].indexCount = 0;
        checkValidation(mesh, 1, false, geo::UvAtlasError::InvalidSubmeshRange,
            "empty submesh range rejected");
    }
    {
        auto mesh = makeTwoSubmeshQuad();
        mesh.indices[0] = 100;
        checkValidation(mesh, 1, false, geo::UvAtlasError::IndexOutOfRange,
            "out-of-range vertex index rejected");
    }
    {
        const auto result = geo::UvAtlasValidator::validate(makeTwoSubmeshQuad(), 2);
        check(!result.valid && result.error == geo::UvAtlasError::UnsupportedUvSet,
            "unsupported UV set rejected");
    }

    // The xatlas modifier output must pass the usable-atlas test
    {
        auto mesh = makeDoubleQuad(true);
        geo::GenerateUv2AtlasModifier mod(&mesh, { .resolution = 256, .paddingPixels = 4 });
        mod.apply();
        checkValidation(mesh, 1, true, geo::UvAtlasError::None,
            "xatlas-generated UV2 atlas accepted");
    }
}

int main(int, char **)
{
    testTwoSubmeshesSharedVertices();
    testCubeRegeneratesWholeObject();
    testExistingUv2Replaced();
    testInvalidInput();
    testPackingFailure();
    testValidator();

    std::cout << (g_failures == 0 ? "ALL TESTS PASSED" : "SOME TESTS FAILED") << std::endl;
    return g_failures == 0 ? 0 : 1;
}
