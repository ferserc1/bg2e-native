//
//  TextureCache.hpp

#pragma once

#include <memory>
#include <unordered_map>
#include <filesystem>

#include <bg2e/render/Texture.hpp>
#include <bg2e/base/Texture.hpp>
#include <bg2e/render/Vulkan.hpp>

namespace bg2e {
namespace utils {

class TextureCache {
public:

    static TextureCache& get();
    static void destroy();

    // Load and catalogues a texture with default settings, unless it is already catalogued, in which case it returns the cached version
    std::shared_ptr<render::Texture> load(render::Vulkan *, const std::filesystem::path& filePath);
    
    std::shared_ptr<render::Texture> load(render::Vulkan *, const std::filesystem::path& filePath, const std::string& fileName);
    
    // Load and catalogues a texture using the supplied Texture object as texture settings. In this case, the Texture object is only used to
    // configure the resulting Texture. The texture image is loaded from the filePath
    std::shared_ptr<render::Texture> load(render::Vulkan *, const std::filesystem::path& filePath, const base::Texture& settings);
    
    std::shared_ptr<render::Texture> load(render::Vulkan *, const std::filesystem::path& filePath, const std::string & fileName, const base::Texture& settings);
    
    // Loads and catalogues a texture, unless it is already catalogued, in which case it returns the cached version. Note that the
    // supplied Texture object must have a valid cacheHash configured (for example, if the texture is loaded from a file, the hash will be
    // the file path)
    std::shared_ptr<render::Texture> load(render::Vulkan *, std::shared_ptr<base::Texture> texture);

    // Remove all objects from the cache
    void emptyCache();
    
protected:
    TextureCache() = default;
    virtual ~TextureCache() = default;
    
    static TextureCache * g_singleton;
    
    std::unordered_map<std::string, std::shared_ptr<render::Texture>> _textures;
};

}
}
