#pragma once

#include <bg2e/draw/all.hpp>
#include <bg2e/scene/Node.hpp>
#include <bg2e/gpu/Device.hpp>
#include <bg2e/gpu/ShaderLib.hpp>
#include <bg2e/gpu/GraphicsPipeline.hpp>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace bg2e::draw::tests {
// Initialized Engine, stopped producers, no active frame. Does not use shaders.
inline void runDrawQueueProbe(Engine& engine)
{
    auto mesh = std::make_shared<Drawable::Mesh>();
    mesh->vertices.resize(6);
    const glm::vec3 positions[6] = {{-0.5f, -0.5f, 0}, {0, -0.5f, 0}, {0, 0.5f, 0}, {0, -0.5f, 0}, {0.5f, -0.5f, 0}, {0.5f, 0.5f, 0}};
    for (size_t i = 0; i < mesh->vertices.size(); ++i)
    {
        mesh->vertices[i] = {positions[i], {0, 0, 1}, {0, 0}, {1, 1}, {1, 0, 0}};
    }
    mesh->indices = {0, 1, 2, 3, 4, 5};
    mesh->submeshes = {{0, 3}, {3, 3}};
    auto drawable = std::make_shared<Drawable>();
    drawable->setMesh(mesh);
    drawable->setTransform(glm::translate(glm::mat4{1}, glm::vec3(4, 0, 0)));
    drawable->setSubmeshTransform(glm::translate(glm::mat4{1}, glm::vec3(1, 0, 0)), 1);
    drawable->material(0).setRoughnessUVSet(1);
    drawable->material(0).setRoughnessChannel(2);
    drawable->material(0).setRoughnessInvert(true);
    drawable->material(0).setRoughnessScale(glm::vec2(2, 3));
    drawable->material(0).setAlphaCutoff(0.3f);
    drawable->load(&engine);
    auto root = std::make_shared<scene::Node>("root");
    root->addComponent(new scene::TransformComponent(glm::translate(glm::mat4{1}, glm::vec3(2, 0, 0))));
    root->addComponent(new scene::EnvironmentComponent());
    auto parent = std::make_shared<scene::Node>("parent");
    const auto parentMatrix = glm::translate(glm::mat4{1}, glm::vec3(3, 0, 0));
    parent->addComponent(new scene::TransformComponent(parentMatrix));
    root->addChild(parent);
    auto child = std::make_shared<scene::Node>("drawable");
    child->addComponent(new scene::DrawableComponent(drawable));
    parent->addChild(child);
    auto disabled = std::make_shared<scene::Node>("disabled");
    disabled->setDisabled(true);
    disabled->addComponent(new scene::DrawableComponent(drawable));
    root->addChild(disabled);
    auto light = std::make_shared<scene::Node>("light");
    light->addComponent(new scene::TransformComponent(glm::translate(glm::mat4{1}, glm::vec3(1, 0, 0))));
    light->addComponent(new scene::LightComponent());
    root->addChild(light);
    RenderQueueVisitor visitor;
    RenderQueue queue;
    visitor.enqueue(root.get(), engine, queue);
    if (queue.items().size() != 2 || queue.queue(RenderQueueType::Opaque).size() != 2 || !queue.environment() ||
        queue.items()[0].worldMatrix[3].x != 9 || queue.items()[1].worldMatrix[3].x != 10 ||
        queue.items()[0].nodeId != child->identifier() || queue.items()[0].objectId != 1 ||
        queue.lights().count.x != 1 || queue.lights().lights[0].positionIntensity.x != 3)
    {
        throw std::runtime_error("Nested draw/light extraction or disabled subtree handling failed");
    }
    const auto& material = queue.items()[0].materialData;
    if (material.uvSets[0].z != 1 || material.channels.y != 2 || material.invert.y != 1 ||
        material.uvScale[1].z != 2 || material.uvScale[1].w != 3 || std::abs(material.emissionAlpha.y - 0.3f) > 0.0001f)
    {
        throw std::runtime_error("Material UV/channel/cutoff ABI packing failed");
    }
    CollectRayTracingInstancesVisitor rays;
    rays.collect(queue);
    const size_t expectedRays = drawable->rayTracingEnabled() ? 2 : 0;
    if (rays.instances().size() != expectedRays)
    {
        throw std::runtime_error("Raster/query extraction disagrees");
    }
    for (size_t i = 0; i < rays.instances().size(); ++i)
    {
        if (rays.instances()[i].instance.instanceId != queue.items()[i].objectId ||
            rays.instances()[i].instance.transform[3].x != queue.items()[i].worldMatrix[3].x)
        {
            throw std::runtime_error("Query identity/world transform differs from raster extraction");
        }
    }
    drawable->material(0).setIsTransparent(true);
    drawable->material(0).setIsSolid(false);
    drawable->material(1).setIsTransparent(true);
    drawable->material(1).setIsSolid(true);
    auto far = std::make_shared<scene::Node>("far instance");
    far->addComponent(new scene::TransformComponent(glm::translate(glm::mat4{1}, glm::vec3(20, 0, 0))));
    far->addComponent(new scene::DrawableComponent(drawable));
    parent->addChild(far);
    RenderQueue mixed;
    visitor.enqueue(root.get(), engine, mixed);
    mixed.sort(glm::vec3{0});
    if (mixed.queue(RenderQueueType::Transparent).size() != 2 || mixed.queue(RenderQueueType::SolidTransparent).size() != 2 ||
        mixed.queue(RenderQueueType::Transparent).front().objectId != 3 || mixed.queue(RenderQueueType::SolidTransparent).front().objectId != 4 ||
        queue.queue(RenderQueueType::Opaque).size() != 2)
    {
        throw std::runtime_error("Queue classification/sort changed object IDs or a previous snapshot");
    }
    rays.collect(mixed);
    if (!rays.instances().empty())
    {
        throw std::runtime_error("Default query collection must exclude both transparent categories");
    }
    bool rejected = false;
    parent->transform()->setMatrix(glm::scale(glm::mat4{1}, glm::vec3{0}));
    try
    {
        visitor.enqueue(root.get(), engine, mixed);
    }
    catch (const std::invalid_argument&)
    {
        rejected = true;
    }
    parent->transform()->setMatrix(parentMatrix);
    if (!rejected || mixed.items().size() != 4)
    {
        throw std::runtime_error("Failed extraction changed the last complete snapshot");
    }
    drawable->setSubmeshVisibility(false, 1);
    visitor.enqueue(root.get(), engine, mixed);
    if (mixed.items().size() != 2)
    {
        throw std::runtime_error("Submesh visibility was ignored after visitor recovery");
    }
    parent->setDisabled(true);
    visitor.enqueue(root.get(), engine, mixed);
    if (!mixed.items().empty())
    {
        throw std::runtime_error("Disabled parent did not exclude descendant instances");
    }
    drawable->cleanup();
}

// Attach to an existing manual runner. Initialize/cleanup at stopped boundaries.
// Retain this object (including per-slot resources) through submitted GPU work.
class BindingProbe {
  public:
    void initialize(Engine& engine, const std::filesystem::path& shaderRoot, gpu::PixelFormat colorFormat, uint32_t frameSlots = 0)
    {
        layout = engine.device()->createPipelineLayout(drawBindingLayout());
        gpu::ShaderLib library(shaderRoot, engine.backendType());
        vertex = library.vertex("draw/bindings/probe", engine.device());
        fragment = library.fragment("draw/bindings/probe", engine.device());
        gpu::GraphicsPipelineDescription description;
        description.layout = layout.get();
        description.vertexShader = vertex.get();
        description.fragmentShader = fragment.get();
        description.colorFormat = colorFormat;
        description.cullMode = gpu::CullMode::None;
        description.depthState = gpu::DepthState{false, false, gpu::CompareOperation::Always};
        description.addVertexBufferDescription(Drawable::DrawMesh::vertexBufferDescription());
        pipeline = engine.device()->createGraphicsPipeline(description);
        frameBinding.initialize(engine, layout, frameSlots);
        objectBinding.initialize(engine, layout, frameSlots);
        lightBinding.initialize(engine, layout, frameSlots);
        environmentBinding.initialize(engine, layout, frameSlots);
    }
    void record(const FrameContext& frame, scene::Node* root, std::shared_ptr<const environment::EnvironmentResources> environment)
    {
        visitor.enqueue(root, frame.engine, queue);
        queue.sort(glm::vec3{0});
        frameBinding.update(frame, glm::mat4{1}, glm::mat4{1});
        objectBinding.prepare(frame, queue);
        lightBinding.update(frame, queue.lights());
        environmentBinding.update(frame, std::move(environment));
        auto& command = frame.commandBuffer;
        command.transition(&frame.colorTarget, gpu::ImageLayout::ColorAttachment);
        command.beginRendering(&frame.colorTarget);
        command.clearColor(0, {0, 0, 0, 1});
        command.bindPipeline(pipeline.get());
        command.bindResourceSet(pipeline.get(), 0, frameBinding.set(frame));
        command.bindResourceSet(pipeline.get(), 2, lightBinding.set(frame));
        command.bindResourceSet(pipeline.get(), 3, environmentBinding.set(frame));
        DrawVisitor draw;
        for (auto type : {RenderQueueType::Opaque, RenderQueueType::Transparent, RenderQueueType::SolidTransparent})
        {
            draw.draw(frame, queue, type, [&](const RenderQueue::Item& item)
            {
                command.bindResourceSet(pipeline.get(), 1, objectBinding.set(frame, item.objectId));
            });
        }
        command.endRendering();
        command.transition(&frame.colorTarget, gpu::ImageLayout::ShaderReadOnly);
    }
    void cleanup()
    {
        frameBinding.cleanup();
        objectBinding.cleanup();
        lightBinding.cleanup();
        environmentBinding.cleanup();
        queue.clear();
        pipeline.reset();
        layout.reset();
        vertex.reset();
        fragment.reset();
    }
  private:
    FrameDataBinding frameBinding;
    ObjectDataBinding objectBinding;
    LightDataBinding lightBinding;
    EnvironmentDataBinding environmentBinding;
    RenderQueueVisitor visitor;
    RenderQueue queue;
    std::shared_ptr<gpu::ShaderModule> vertex;
    std::shared_ptr<gpu::ShaderModule> fragment;
    std::shared_ptr<gpu::PipelineLayout> layout;
    std::shared_ptr<gpu::GraphicsPipeline> pipeline;
};
}
