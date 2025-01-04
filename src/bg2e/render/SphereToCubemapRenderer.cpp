
#include <bg2e/render/SphereToCubemapRenderer.hpp>

namespace bg2e::render {

SphereToCubemapRenderer::SphereToCubemapRenderer(Vulkan * vulkan)
    :CubemapRenderer(vulkan)
{

}


void SphereToCubemapRenderer::build(
    const std::filesystem::path& imagePath,
    VkExtent2D cubeImageSize
) {
    CubemapRenderer::build(
        imagePath,
        "sphere_to_cubemap.vert.spv",
        "sphere_to_cubemap.frag.spv",
        cubeImageSize
    );
}

}
