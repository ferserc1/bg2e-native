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
#include <bg2e.hpp>

class MyOffscreenApplication : public bg2e::app::OffscreenApplicationDelegate
{
public:
    void initConfig(
        [[maybe_unused]] int argc, [[maybe_unused]] char ** argv,
        bg2e::app::OffscreenApplicationConfig & outConfig
    ) override {
        outConfig.width = 1920;
        outConfig.height = 1080;
        outConfig.createColorImage = true;
        outConfig.createDepthImage = true;
    }

    void init(
        bg2e::render::Engine* engine,
        std::shared_ptr<bg2e::render::vulkan::Image> colorImage,std::shared_ptr<bg2e::render::vulkan::Image> depthImage
    ) override {
        _engine = engine;
        _colorImage = colorImage;
        _depthImage = depthImage;
    }

    void initFrameResources(bg2e::render::vulkan::DescriptorSetAllocator* allocator) override
    {
        allocator->requirePoolSizeRatio(1, {
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2 },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 }
        });
        allocator->initPool();
    }

    void initScene() override
    {
        auto assetPath = bg2e::base::PlatformTools::assetPath();
        auto texture = new bg2e::base::Texture(assetPath, "two_submeshes_inner_albedo.jpg");
        texture->setMagFilter(bg2e::base::Texture::FilterLinear);
        texture->setMinFilter(bg2e::base::Texture::FilterLinear);
        _texture = std::shared_ptr<bg2e::render::Texture>(new bg2e::render::Texture(
            _engine,
            texture
        ));

        auto texture2 = new bg2e::base::Texture(assetPath, "two_submeshes_outer_albedo.jpg");
        texture2->setMagFilter(bg2e::base::Texture::FilterLinear);
        texture2->setMinFilter(bg2e::base::Texture::FilterLinear);

        _texture2 = std::shared_ptr<bg2e::render::Texture>(new bg2e::render::Texture(
            _engine,
            texture2
        ));

        _engine->cleanupManager().push([&](VkDevice) {
            _texture.reset();
            _texture2.reset();
        });

        createPipeline();

        _sceneData.viewMatrix = glm::lookAt(glm::vec3{ 0.0f, 0.0f, -5.0f}, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f });

        auto vpSize = _colorImage->extent2D();
        _sceneData.projMatrix = glm::perspective(
            glm::radians(50.0f),
            float(vpSize.width) / float(vpSize.height),
            0.1f, 40.0f
        );

        _objectData.modelMatrix = glm::mat4{ 1.0f };

        createVertexData();
    }

    void resize(uint32_t width, uint32_t height) override
    {
        _sceneData.projMatrix = glm::perspective(
            glm::radians(50.0f),
            float(width) / float(height),
            0.1f, 40.0f
        );
    }

    void frame(float delta, uint32_t frameIndex, bg2e::render::vulkan::FrameResources& frameResources) override
    {
    }

    bool render(
        VkCommandBuffer cmd,
        uint32_t frameIndex,
        bg2e::render::vulkan::FrameResources& frameResources,
        VkImageLayout colorImageLayout,
        VkImageLayout& finalColorImageLayout
    ) override {
        using namespace bg2e::render::vulkan;
        auto sceneDS = macros::uniformBufferDescriptorSet(
            _engine, frameResources,
            _sceneDSLayout, _sceneData, frameIndex
        );

        float flash = std::abs(std::sin(frameIndex / 120.0f));
        VkClearColorValue clearValue{ { 0.0f, 0.0f, flash, 1.0f } };
        auto clearRange = Image::subresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);
        vkCmdClearColorImage(
            cmd,
            _colorImage->handle(),
            VK_IMAGE_LAYOUT_GENERAL,
            &clearValue, 1, &clearRange
        );

        auto colorAttachment = Info::attachmentInfo(_colorImage->imageView(), nullptr);
        auto depthAttachment = Info::depthAttachmentInfo(_depthImage->imageView());
        auto renderInfo = Info::renderingInfo(_colorImage->extent2D(), &colorAttachment, &depthAttachment);
        cmdBeginRendering(cmd, &renderInfo);

        macros::cmdSetDefaultViewportAndScissor(cmd, _colorImage->extent2D());

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline);

        _objectData.modelMatrix = glm::rotate(_objectData.modelMatrix, delta() * 0.001f, glm::vec3{ 0.0f, 1.0f, 0.0f });
        auto objectDataBuffer = macros::createBuffer(_engine, frameResources, _objectData);

        for (uint32_t i = 0; i < _mesh->submeshCount(); ++i)
        {
            auto objectDS = frameResources.newDescriptorSet(_objectDSLayout);
            auto texture = i == 0 ? _texture2.get() : _texture.get();
            objectDS->beginUpdate();
            objectDS->addBuffer(
                0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                objectDataBuffer, sizeof(ObjectData), 0
            );
            objectDS->addImage(
                1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                texture->image()->imageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                _texture->sampler()
            );
            objectDS->endUpdate();

            std::array<VkDescriptorSet, 2> sets = {
                sceneDS->descriptorSet(),
                objectDS->descriptorSet()
            };

            vkCmdBindDescriptorSets(
                cmd,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                _layout, 0,
                uint32_t(sets.size()),
                sets.data(),
                0, nullptr
            );
            _mesh->drawSubmesh(cmd, i);
        }

        cmdEndRendering(cmd);
        return false;
    }

    void didRenderFrame(
        uint32_t frameIndex,
        double elapsedMs,
        VkImageLayout colorImageLayout
    ) override {
        auto width = _colorImage->extent2D().width;
        auto height = _colorImage->extent2D().height;
        size_t imageSize = width * height * 4;
        std::vector<uint8_t> outData(imageSize);
        bg2e::render::vulkan::Image::readPixelsRGBA8(
            _engine,
            _colorImage.get(),
            0, 0,
            _colorImage->extent2D().width, _colorImage->extent2D().height,
            outData,
            colorImageLayout
        );

        auto homePath = bg2e::base::PlatformTools::homePath();
        bg2e::db::saveImage(homePath / "out.jpg", outData.data(), width, height, 4);
    }

protected:
    bg2e::render::Engine * _engine;
    std::shared_ptr<bg2e::render::vulkan::Image> _colorImage;
    std::shared_ptr<bg2e::render::vulkan::Image> _depthImage;

    VkPipelineLayout _layout;
    VkPipeline _pipeline;

    std::unique_ptr<bg2e::render::vulkan::geo::MeshPU> _mesh;

    std::shared_ptr<bg2e::render::Texture> _texture;
    std::shared_ptr<bg2e::render::Texture> _texture2;

    VkDescriptorSetLayout _sceneDSLayout;
    VkDescriptorSetLayout _objectDSLayout;

    struct SceneData
    {
        glm::mat4 viewMatrix;
        glm::mat4 projMatrix;
    };

    SceneData _sceneData;

    struct ObjectData
    {
        glm::mat4 modelMatrix;
    };
    ObjectData _objectData;

    void createPipeline()
	{
		bg2e::render::vulkan::factory::GraphicsPipeline plFactory(_engine);

		plFactory.addShader("offscreen_render_cli/texture.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		plFactory.addShader("offscreen_render_cli/texture.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

        auto bindingDescription = bg2e::render::vulkan::geo::bindingDescriptionPU();
        auto attributeDescriptions = bg2e::render::vulkan::geo::attributeDescriptionsPU();

		plFactory.vertexInputState.vertexBindingDescriptionCount = 1;
		plFactory.vertexInputState.pVertexBindingDescriptions = &bindingDescription;
		plFactory.vertexInputState.vertexAttributeDescriptionCount = uint32_t(attributeDescriptions.size());
		plFactory.vertexInputState.pVertexAttributeDescriptions = attributeDescriptions.data();

        bg2e::render::vulkan::factory::DescriptorSetLayout dsFactory;

        dsFactory.clear();
        dsFactory.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        _sceneDSLayout = dsFactory.build(_engine->device().handle(), VK_SHADER_STAGE_VERTEX_BIT);

        dsFactory.clear();
        dsFactory.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        dsFactory.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        _objectDSLayout = dsFactory.build(_engine->device().handle(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);


		auto layoutInfo = bg2e::render::vulkan::Info::pipelineLayoutInfo();
        VkDescriptorSetLayout setLayouts[] = {
            _sceneDSLayout,
            _objectDSLayout
        };
        layoutInfo.pSetLayouts = setLayouts;
        layoutInfo.setLayoutCount = 2;
		VK_ASSERT(vkCreatePipelineLayout(_engine->device().handle(), &layoutInfo, nullptr, &_layout));

        plFactory.setDepthFormat(_depthImage->format());
        plFactory.enableDepthtest(true, VK_COMPARE_OP_LESS);
        plFactory.inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        plFactory.setCullMode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
		plFactory.setColorAttachmentFormat(_colorImage->format());
		_pipeline = plFactory.build(_layout);

		_engine->cleanupManager().push([&](VkDevice dev) {
			vkDestroyPipeline(dev, _pipeline, nullptr);
			vkDestroyPipelineLayout(dev, _layout, nullptr);
            vkDestroyDescriptorSetLayout(dev, _sceneDSLayout, nullptr);
            vkDestroyDescriptorSetLayout(dev, _objectDSLayout, nullptr);
		});
	}

	void createVertexData()
	{
		using namespace bg2e::render::vulkan;

        auto mesh = std::unique_ptr<bg2e::geo::MeshPU>(
            bg2e::db::loadMeshObj<bg2e::geo::MeshPU>(bg2e::base::PlatformTools::assetPath().append("two_submeshes.obj"))
        );

        _mesh = std::unique_ptr<bg2e::render::vulkan::geo::MeshPU>(new bg2e::render::vulkan::geo::MeshPU(_engine));
        _mesh->setMeshData(mesh.get());

		_mesh->build();

		_engine->cleanupManager().push([this](VkDevice dev) {
			_mesh.reset();
		});
	}
};

int main(int argc, char** argv)
{
    bg2e_log_info << "bg2 engine offscreen render CLI example" << bg2e_log_end;

    bg2e::app::OffscreenApplication app;
    app.init(
        argc, argv,
        "my-offscreen-app",
        std::make_shared<MyOffscreenApplication>()
    );

    return app.run();
}

