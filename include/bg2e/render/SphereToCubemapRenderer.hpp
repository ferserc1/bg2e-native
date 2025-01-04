
#pragma once

#include <bg2e/render/CubemapRenderer.hpp>

namespace bg2e {
namespace render {

class BG2E_API SphereToCubemapRenderer : public CubemapRenderer {
public:
    SphereToCubemapRenderer(Vulkan * vulkan);

    void build(
        const std::filesystem::path& imagePath,
        VkExtent2D cubeImageSize = { 1024, 1024 }
    );
};

}
}
