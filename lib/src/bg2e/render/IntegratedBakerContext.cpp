#include <bg2e/render/IntegratedBakerContext.hpp>
#include <bg2e/render/Engine.hpp>

#include <stdexcept>
#include <utility>

namespace bg2e {
namespace render {

IntegratedLightmapBaker::IntegratedLightmapBaker(
    std::shared_ptr<BakerContext> context,
    std::shared_ptr<scene::Node> targetNode,
    LightmapSettings settings)
    : LightmapBaker(std::move(context), std::move(targetNode), settings)
{}

void IntegratedLightmapBaker::update(VkCommandBuffer cmd, vulkan::FrameResources&)
{
    recordUvSurface(cmd, _context->engine()->currentFrameResourcesIndex());
}

IntegratedBakerContext::IntegratedBakerContext(Engine* engine, scene::Node* rootNode)
    : BakerContext(engine, rootNode)
{}

std::unique_ptr<IntegratedLightmapBaker> IntegratedBakerContext::createBaker(
    std::shared_ptr<scene::Node> targetNode,
    LightmapSettings settings)
{
    return std::unique_ptr<IntegratedLightmapBaker>(
        new IntegratedLightmapBaker(shared_from_this(), std::move(targetNode), settings));
}

}
}
