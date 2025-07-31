//
//  scene.cpp

#include <bg2e/db/scene.hpp>
#include <bg2e/json/JsonNode.hpp>

#include <fstream>

namespace bg2e::db {

bg2e::scene::Node * loadScene(
    const std::filesystem::path& filePath
) {
    return nullptr;
}

bg2e::scene::Node * loadScene(
    const std::filesystem::path& basePath,
    const std::string& fileName
) {
    return nullptr;
}

void saveScene(
    bg2e::scene::Node * sceneRoot,
    const std::filesystem::path& filePath
) {
    auto rootPath = filePath;
    rootPath.remove_filename();
    auto sceneData = sceneRoot->serialize(rootPath);
    
    std::ofstream file;
    file.open(filePath);
    if (file.is_open())
    {
        using namespace bg2e::json;
        auto sceneRoot = JSON(JsonObject{
            { "fileType", JSON("bg2e::scene") },
            { "version", JSON(JsonObject{
                { "major", JSON(1) },
                { "minor", JSON(0) },
                { "rev", JSON(0) },
            })},
            { "scene", JSON(JsonList{sceneData})}
        });
        file << sceneRoot->toString();
        file.close();
    }
}

void saveScene(
    bg2e::scene::Node * sceneRoot,
    const std::filesystem::path& basePath,
    const std::string& fileName
) {
    auto fullPath = basePath;
    fullPath += fileName;
    saveScene(sceneRoot, fullPath);
}

}
