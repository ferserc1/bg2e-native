#pragma once

#include <bg2e/render/ColorAttachments.hpp>
#include <bg2e/scene/vk/FrameDataBinding.hpp>
#include <bg2e/scene/vk/ObjectDataBinding.hpp>
#include <bg2e/scene/vk/EnvironmentDataBinding.hpp>
#include <bg2e/render/EnvironmentResources.hpp>
#include <bg2e/scene/ResizeViewportVisitor.hpp>
#include <bg2e/scene/UpdateVisitor.hpp>
#include <bg2e/scene/DrawVisitor.hpp>
#include <bg2e/scene/Scene.hpp>
#include <bg2e/scene/Node.hpp>
#include <bg2e/render/Engine.hpp>
#include <bg2e/render/Renderer.hpp>

#include <memory>

namespace bg2e {
namespace render {

// TODO: Refactor this class to be more generic, and implement a specific renderer for each rendering technique (forward, deferred, etc.)
class BG2E_API RendererBasicForward : public Renderer {
public:
    RendererBasicForward() = default;
    virtual ~RendererBasicForward() = default;

    inline VkFormat colorAttachmentFormat() const { return _colorAttachmentFormat; }
    inline void setColorAttachmentFormat(VkFormat format) { _colorAttachmentFormat = format; }

    inline VkFormat depthAttachmentFormat() const { return _depthAttachmentFormat; }
    inline void setDepthAttachmentFormat(VkFormat format) { _depthAttachmentFormat = format; }
    
    inline VkPipeline pipeline() const { return _pipeline; }
    inline VkPipelineLayout pipelineLayout() const { return _pipelineLayout; }

    inline const bg2e::scene::vk::FrameDataBinding* frameDataBinding() const { return _frameDataBinding.get(); }
    inline const bg2e::scene::vk::ObjectDataBinding* objectDataBinding() const { return _objectDataBinding.get(); }
    inline const bg2e::scene::vk::EnvironmentDataBinding* environmentDataBinding() const { return _environmentDataBinding.get(); }
    inline const bg2e::render::ColorAttachments* colorAttachments() const { return _colorAttachments.get(); }

    inline bg2e::scene::vk::FrameDataBinding* frameDataBinding() { return _frameDataBinding.get(); }
    inline bg2e::scene::vk::ObjectDataBinding* objectDataBinding() { return _objectDataBinding.get(); }
    inline bg2e::scene::vk::EnvironmentDataBinding* environmentDataBinding() { return _environmentDataBinding.get(); }

    inline bool drawSkybox() const { return _drawSkybox; }
    inline void setDrawSkybox(bool value) { _drawSkybox = value; }

    // This renderer is a prototype for a simple forward renderer. In the future, it will be possible to implement other rendering techniques by inheriting from this class and overriding the necessary methods.
    void build(
        bg2e::render::Engine* engine
    ) override;

    void initFrameResources(
        bg2e::render::vulkan::DescriptorSetAllocator* frameAllocator
    ) override;

    void initScene(
        std::shared_ptr<bg2e::scene::Node> sceneRoot
    ) override;

    void resize(
        VkExtent2D newExtent
    ) override;
    void update(
        float delta
    ) override;
    void draw(
        VkCommandBuffer cmd,
        uint32_t currentFrame,
        const bg2e::render::vulkan::Image* depthImage,
        bg2e::render::vulkan::FrameResources& frameResources
    ) override;

    void cleanup() override;

    void createPipeline(bg2e::render::Engine* engine);

protected:

    bg2e::render::Engine* _engine = nullptr;

    // Specific for forward rendering one unique color attachment and a depth attachment
    VkFormat _colorAttachmentFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
    VkFormat _depthAttachmentFormat = VK_FORMAT_D32_SFLOAT;

    std::unique_ptr<bg2e::render::ColorAttachments> _colorAttachments;

    std::unique_ptr<bg2e::scene::vk::FrameDataBinding> _frameDataBinding;
    std::unique_ptr<bg2e::scene::vk::ObjectDataBinding> _objectDataBinding;
    std::unique_ptr<bg2e::scene::vk::EnvironmentDataBinding> _environmentDataBinding;

    std::unique_ptr<bg2e::render::EnvironmentResources> _environment;
    size_t _skyImageHash;

    std::unique_ptr<bg2e::scene::Scene> _scene;

    bg2e::scene::ResizeViewportVisitor _resizeVisitor;
    bg2e::scene::UpdateVisitor _updateVisitor;
    bg2e::scene::DrawVisitor _drawVisitor;

    VkPipeline _pipeline = VK_NULL_HANDLE;
    VkPipelineLayout _pipelineLayout = VK_NULL_HANDLE;

    bool _drawSkybox = true;
};

}
}
