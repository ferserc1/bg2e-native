//
// Created by fernando on 3/12/25.
//
#include <bg2e/db/scene_gltf.hpp>

#include <iostream>

#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/types.hpp>

namespace bg2e::db {

void dumpNodeRecursive(const fastgltf::Asset& asset,
                       std::size_t nodeIndex,
                       int depth = 0)
{
    const auto& node = asset.nodes[nodeIndex];

    // Sangrado para ver jerarquía
    for (int i = 0; i < depth; ++i)
        std::cout << "  ";

    std::cout << "Node " << nodeIndex;

    if (!node.name.empty())
        std::cout << " (\"" << node.name << "\")";

    if (node.meshIndex.has_value())
        std::cout << "  mesh=" << *node.meshIndex;

    std::cout << "\n";

    // Hijos
    for (auto childIndex : node.children) {
        dumpNodeRecursive(asset, childIndex, depth + 1);
    }
}

void dumpGltf(const std::filesystem::path& path)
{
    fastgltf::Parser parser;

    // Cargar bytes del fichero en el buffer de fastgltf
    auto maybeBuffer = fastgltf::GltfDataBuffer::FromPath(path);
    if (maybeBuffer.error() != fastgltf::Error::None) {
        std::cerr << "Error cargando fichero: " << path << "\n";
        return;
    }
    fastgltf::GltfDataBuffer* data = maybeBuffer.get_if();
    fastgltf::GltfDataBuffer& buffer = *data;

    // Solo queremos la estructura, así que Options::None
    auto assetResult = parser.loadGltf(
        buffer,
        path.parent_path(),
        fastgltf::Options::None
    );

    if (auto error = assetResult.error(); error != fastgltf::Error::None) {
        std::cerr << "Error al parsear glTF: "
                  << static_cast<std::uint64_t>(error) << "\n";
        return;
    }

    // Referencia al Asset ya cargado
    const fastgltf::Asset& asset = assetResult.get();

    std::cout << "File: " << path << "\n\n";

    // Info general
    std::cout << "Scenes:  " << asset.scenes.size()  << "\n";
    std::cout << "Nodes:   " << asset.nodes.size()   << "\n";
    std::cout << "Meshes:  " << asset.meshes.size()  << "\n";
    std::cout << "Buffers: " << asset.buffers.size() << "\n\n";

    // Escenas
    for (std::size_t i = 0; i < asset.scenes.size(); ++i) {
        const auto& scene = asset.scenes[i];
        std::cout << "Scene " << i;
        if (!scene.name.empty())
            std::cout << " (\"" << scene.name << "\")";
        std::cout << "\n";

        // Nodos raíz de la escena
        for (auto nodeIndex : scene.nodeIndices) {
            dumpNodeRecursive(asset, nodeIndex, 1);
        }

        std::cout << "\n";
    }

    // Mallas
    for (std::size_t i = 0; i < asset.meshes.size(); ++i) {
        const auto& mesh = asset.meshes[i];
        std::cout << "Mesh " << i;
        if (!mesh.name.empty())
            std::cout << " (\"" << mesh.name << "\")";
        std::cout << "  primitives=" << mesh.primitives.size() << "\n";
    }
}

extern BG2E_API bg2e::scene::Node * loadGltf(
    const std::filesystem::path& filePath
) {
    dumpGltf(filePath);
    std::cerr << "loadGltf: Not implemented" << std::endl;
    return nullptr;
}

extern BG2E_API bg2e::scene::Node * loadGltf(
    const std::filesystem::path& basePath,
    const std::string& fileName
) {
    auto fullPath = basePath / fileName;
    return loadGltf(fullPath);
}

}
