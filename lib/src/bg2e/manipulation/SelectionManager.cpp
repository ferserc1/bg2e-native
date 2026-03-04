//
//  SelectionManager.cpp

#include <bg2e/manipulation/SelectionManager.hpp>

#include <bg2e/render/all.hpp>

#include "bg2e/manipulation/SelectableComponent.hpp"

namespace bg2e::manipulation {

SelectionManager::SelectionManager(render::Engine * engine)
    : _engine(engine)
{

}

void SelectionManager::init()
{
    createImage();
    createPipeline();
    
    _pickVisitor = std::make_shared<manipulation::PickSelectionVisitor>();
}

bool SelectionManager::pick(
    scene::Node * rootNode,
    const glm::mat4 & viewMatrix,
    const glm::mat4 & projMatrix,
    const math::Viewport & vp,
    uint32_t x,
    uint32_t y
) {
    SelectionItem result {};
    if (pickObject(rootNode, viewMatrix, projMatrix, vp, x, y, &result))
    {
        if (isSelected(result.node, result.drawable, result.submesh))
        {
            removeFromSelectedItems(result.node, result.submesh);
        }
        else
        {
            addToSelectedItems(result.node, result.drawable, result.submesh);
        }
        return true;
    }
    else if (_clearSelectionOnEmptyPick)
    {
        deselect();
    }

    return false;
}

bool SelectionManager::pickObject(
    scene::Node * rootNode,
    const glm::mat4 & viewMatrix,
    const glm::mat4 & projMatrix,
    const math::Viewport & vp,
    uint32_t x,
    uint32_t y,
    SelectionItem * result
) {
    if (!result)
    {
        throw std::runtime_error("SelectionManager::pickObject: result is null");
    }

    auto objectId = pickObjectId(rootNode, viewMatrix, projMatrix, vp, x, y);
    if (auto objectData = _pickVisitor->findObject(objectId))
    {
        result->node = objectData->node;
        result->drawable = objectData->node->getComponent<scene::DrawableComponent>();
        result->mesh = result->drawable->drawable().get();
        result->submesh = objectData->submeshIndex;
        return true;
    }

    return false;
}

void SelectionManager::deselect()
{
    for (const auto & item : _selectedItems)
    {
        if (const auto& sel = item->node->getComponent<SelectableComponent>())
        {
            sel->setSelected(item->submesh, false);
        }
    }

    _selectedItems.clear();
}

bool SelectionManager::isSelected(const scene::Node * node, const scene::DrawableComponent * drawable, uint32_t submesh)
{
    return isSelected(node, drawable->drawable().get(), submesh);
}

bool SelectionManager::isSelected(const scene::Node * node, const scene::Drawable * drawable, uint32_t submesh)
{
    if (!node || !drawable)
    {
        return false;
    }

    return std::find_if(
        _selectedItems.cbegin(),
        _selectedItems.cend(),
        [node, drawable, submesh](const std::shared_ptr<SelectionItem>& curItem)
        {
            return node == curItem->node && drawable == curItem->mesh && submesh == curItem->submesh;
        }
    ) != _selectedItems.cend();
}

bool SelectionManager::addToSelectedItems(scene::Node * node, scene::DrawableComponent * drawable, uint32_t submesh)
{
    // Are the pointets valid?
    if (!node || !drawable || !drawable->drawable().get())
    {
        return false;
    }

    // Is the node selectable?
    auto selComponent = node->getComponent<SelectableComponent>();
    if (!selComponent)
    {
        return false;
    }

    // Is the submesh index valid?
    auto mesh = drawable->drawable().get();
    if (submesh >= mesh->submeshesCount())
    {
        return false;
    }

    // If the node is already selected, return true
    if (isSelected(node, drawable, submesh))
    {
        return true;
    }

    // If multi selection is disabled, deselect all items
    if (!multiSelection())
    {
        deselect();
    }

    // Build the SelectionItem class
    auto selectItem = std::make_shared<SelectionItem>();
    selectItem->node = node;
    selectItem->drawable = drawable;
    selectItem->mesh = mesh;
    selectItem->submesh = submesh;
    _selectedItems.push_back(selectItem);

    // Mark the submesh as selected
    selComponent->setSelected(submesh, true);

    return true;
}

void SelectionManager::removeFromSelectedItems(scene::Node * node)
{
    if (!node)
    {
        throw std::runtime_error("SelectionManager: invalid node pointer supplied calling removeFromSelectedItems()");
    }
    std::erase_if(
        _selectedItems,
        [node](const std::shared_ptr<SelectionItem>& curItem)
        {
            if (curItem->node == node)
            {
                auto sel = curItem->node->getComponent<SelectableComponent>();
                if (!sel)
                {
                    return false;
                }
                sel->setSelected(curItem->submesh, false);
                return true;
            }
            return false;
        }
    );
}

void SelectionManager::removeFromSelectedItems(scene::Node * node, uint32_t submesh)
{
    const auto drawable = const_cast<scene::DrawableComponent*>(node->getComponent<scene::DrawableComponent>());
    return removeFromSelectedItems(drawable, submesh);
}

void SelectionManager::removeFromSelectedItems(scene::DrawableComponent* drawable, uint32_t submesh)
{
    if (!drawable || !drawable->ownerNode())
    {
        throw std::runtime_error("SelectionManager: invalid drawable pointer supplied calling removeFromSelectedItems()");
    }
    auto node = drawable->ownerNode();
    std::erase_if(
        _selectedItems,
        [drawable, submesh](const std::shared_ptr<SelectionItem>& curItem)
        {
            auto sel = curItem->node->getComponent<SelectableComponent>();
            if (!sel)
            {
                return false;
            }
            if (curItem->drawable == drawable && curItem->submesh == submesh)
            {
                sel->setSelected(curItem->submesh, false);
                return true;
            }
            return false;
        }
    );
}

uint32_t SelectionManager::pickObjectId(
    scene::Node * rootNode,
    const glm::mat4 & viewMatrix,
    const glm::mat4 & projMatrix,
    const math::Viewport & vp,
    uint32_t x,
    uint32_t y
) {
    using namespace render::vulkan;

    if (_image->extent2D().width != static_cast<uint32_t>(vp.width) ||
        _image->extent2D().height != static_cast<uint32_t>(vp.height)
    ) {
        cleanupImage();
        createImage();
    }

    _selectedItem.reset();

    _engine->command().immediateSubmit([&](VkCommandBuffer cmd) {
        Image::cmdTransitionImage(
            cmd,
            _image->handle(),
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_GENERAL
        );
        VkClearColorValue clearValue { { 0.0f, 0.0f, 0.0f, 0.0f } };
        auto clearRange = Image::subresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);
        vkCmdClearColorImage(
            cmd,
            _image->handle(), VK_IMAGE_LAYOUT_GENERAL,
            &clearValue, 1, &clearRange
        );

        Image::cmdTransitionImage(
            cmd,
            _image->handle(),
            VK_IMAGE_LAYOUT_GENERAL,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
        );

        auto colorAttachment = Info::attachmentInfo(_image->imageView(), nullptr);
        auto depthAttachment = Info::depthAttachmentInfo(_engine->swapchain().depthImage()->imageView());
        auto renderInfo = Info::renderingInfo(_image->extent2D(), &colorAttachment, &depthAttachment);

        cmdBeginRendering(cmd, &renderInfo);

        macros::cmdSetDefaultViewportAndScissor(cmd, _image->extent2D());

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline);

        _pickVisitor->pick(rootNode, viewMatrix, projMatrix, cmd, _pipelineLayout);

        cmdEndRendering(cmd);
    });

    // Read pixels in image
    std::vector<uint8_t> outData = { 0, 0, 0, 0 };
    Image::readPixelsRGBA8(
        _engine,
        _image.get(),
        x, y,
        1, 1,
        outData,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    );
    return SelectableComponent::decodeObjectId(outData.data());
}

void SelectionManager::createImage()
{
    using namespace bg2e::render::vulkan;

    auto extent = _engine->swapchain().extent();
    _image = std::shared_ptr<render::vulkan::Image>(
        render::vulkan::Image::createAllocatedImage(
            _engine,
            VK_FORMAT_R8G8B8A8_UNORM,
            extent,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT)
    );
    
    _engine->cleanupManager().push([&](VkDevice) {
        _image.reset();
    });
}

void SelectionManager::createPipeline()
{
    using namespace bg2e::render::vulkan;
    factory::GraphicsPipeline plFactory(_engine);
    
    plFactory.addShader("pick_selection.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    plFactory.addShader("pick_selection.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
    
    VkPushConstantRange pushConstantRange = {};
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(PushConstantData);
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    
    auto layoutInfo = Info::pipelineLayoutInfo();
    layoutInfo.pPushConstantRanges = &pushConstantRange;
    layoutInfo.pushConstantRangeCount = 1;
    
    VK_ASSERT(vkCreatePipelineLayout(_engine->device().handle(), &layoutInfo, nullptr, &_pipelineLayout));
    
    plFactory.setInputState<render::vulkan::geo::Mesh>();
    plFactory.setColorAttachmentFormat(_image->format());
    plFactory.setDepthFormat(_engine->swapchain().depthImageFormat());
    plFactory.enableDepthtest(true, VK_COMPARE_OP_LESS);
    plFactory.inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    plFactory.setCullMode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    plFactory.multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    _pipeline = plFactory.build(_pipelineLayout);
    
    _engine->cleanupManager().push([&](VkDevice dev) {
        vkDestroyPipeline(dev, _pipeline, nullptr);
        vkDestroyPipelineLayout(dev, _pipelineLayout, nullptr);
    });
}

void SelectionManager::cleanupImage()
{
    _image.reset();
}
}
