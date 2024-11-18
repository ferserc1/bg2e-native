
#include <bg2e/app/MainLoop.hpp>
#include <bg2e/app/Application.hpp>
#include <bg2e/ui/UserInterface.hpp>
#include <bg2e/render/vulkan/Image.hpp>
#include <bg2e/base/Log.hpp>
#include <bg2e/render/vulkan/factory/GraphicsPipeline.hpp>
#include <bg2e/render/vulkan/geo/VertexDescription.hpp>
#include <bg2e/render/vulkan/Info.hpp>
#include <bg2e/render/vulkan/macros/graphics.hpp>
#include <bg2e/geo/Vertex.hpp>

#include <bg2e/ui/BasicWidgets.hpp>
#include <bg2e/ui/Window.hpp>

class ClearScreenDelegate : public bg2e::render::RenderLoopDelegate,
	public bg2e::app::InputDelegate,
	public bg2e::ui::UserInterfaceDelegate
{
public:
	void init(bg2e::render::Vulkan* vulkan) override
	{
		using namespace bg2e::render::vulkan;
		RenderLoopDelegate::init(vulkan);

		createImage(vulkan->swapchain().extent());
  
        createPipeline();

		createVertexData();
	}

	void swapchainResized(VkExtent2D newExtent) override
	{
		_targetImage->cleanup();
		createImage(newExtent);
	}

	VkImageLayout render(
		VkCommandBuffer cmd,
		uint32_t currentFrame,
		const bg2e::render::vulkan::Image* colorImage,
		const bg2e::render::vulkan::Image* depthImage,
		bg2e::render::vulkan::FrameResources& frameResources
	) override {
		using namespace bg2e::render::vulkan;

		Image::cmdTransitionImage(
			cmd,
			_targetImage->handle(),
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_GENERAL
		);

		float flash = std::abs(std::sin(currentFrame / 120.0f));
		VkClearColorValue clearValue{ { 0.0f, 0.0f, flash, 1.0f } };
		auto clearRange = Image::subresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);
		vkCmdClearColorImage(
			cmd,
			_targetImage->handle(),
			VK_IMAGE_LAYOUT_GENERAL,
			&clearValue, 1, &clearRange
		);

		Image::cmdTransitionImage(
			cmd, _targetImage->handle(),
			VK_IMAGE_LAYOUT_GENERAL,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		);

		auto colorAttachment = Info::attachmentInfo(_targetImage->imageView(), nullptr);
		auto renderInfo = Info::renderingInfo(_targetImage->extent2D(), &colorAttachment, nullptr);
		cmdBeginRendering(cmd, &renderInfo);

		macros::cmdSetDefaultViewportAndScissor(cmd, _targetImage->extent2D());

		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline);

		VkBuffer vertexBuffers[] = { _vertexBuffer->handle() };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);

		vkCmdBindIndexBuffer(cmd, _indexBuffer->handle(), 0, VK_INDEX_TYPE_UINT16);

		vkCmdDrawIndexed(cmd, uint32_t(_indices.size()), 1, 0, 0, 0);

		bg2e::render::vulkan::cmdEndRendering(cmd);

		Image::cmdTransitionImage(
			cmd,
			_targetImage->handle(),
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
		);
		Image::cmdTransitionImage(
			cmd,
			colorImage->handle(),
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
		);

		Image::cmdCopy(
			cmd,
			_targetImage->handle(), _targetImage->extent2D(),
			colorImage->handle(), colorImage->extent2D()
		);
  
		return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	}

	// ============ User Interface Delegate Functions =========
	void init(bg2e::render::Vulkan*, bg2e::ui::UserInterface*) override {
		_window.setTitle("ImGui Wrapper Demo");
		_window.options.noClose = true;
        _window.options.minWidth = 100;
        _window.options.minHeight = 50;
        _window.setPosition(0, 0);
        _window.setSize(150, 90);
	}

	void drawUI() override
	{
		using namespace bg2e::ui;
		_window.draw([]() {
			BasicWidgets::text("Hello, world!");
		});
	}

	void cleanup() override
	{
		_targetImage->cleanup();
	}

protected:
	std::shared_ptr<bg2e::render::vulkan::Image> _targetImage;

	bg2e::ui::Window _window;
 
    VkPipelineLayout _layout;
    VkPipeline _pipeline;

	const std::vector<bg2e::geo::VertexPC> _vertices = {
		{ { -0.5f, -0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
		{ {  0.5f, -0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
		{ {  0.5f,  0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } },
		{ { -0.5f,  0.5f, 0.0f }, { 1.0f, 0.0f, 1.0f, 1.0f } }
	};

	const std::vector<uint16_t> _indices = { 0, 1, 2, 2, 3, 0 };

	std::unique_ptr<bg2e::render::vulkan::Buffer> _vertexBuffer;
	std::unique_ptr<bg2e::render::vulkan::Buffer> _indexBuffer;
    
    void createPipeline()
    {
        bg2e::render::vulkan::factory::GraphicsPipeline plFactory(_vulkan);
        
        plFactory.addShader("test.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
        plFactory.addShader("test.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

		auto bindingDescription = bg2e::render::vulkan::geo::bindingDescription<bg2e::geo::VertexPC>();
		auto attributeDescriptions = bg2e::render::vulkan::geo::attributeDescriptions<bg2e::geo::VertexPC>();

		plFactory.vertexInputState.vertexBindingDescriptionCount = 1;
		plFactory.vertexInputState.pVertexBindingDescriptions = &bindingDescription;
		plFactory.vertexInputState.vertexAttributeDescriptionCount = uint32_t(attributeDescriptions.size());
		plFactory.vertexInputState.pVertexAttributeDescriptions = attributeDescriptions.data();
        
        auto layoutInfo = bg2e::render::vulkan::Info::pipelineLayoutInfo();
        VK_ASSERT(vkCreatePipelineLayout(_vulkan->device().handle(), &layoutInfo, nullptr, &_layout));
        plFactory.setColorAttachmentFormat(_targetImage->format());
        _pipeline = plFactory.build(_layout);
        
        _vulkan->cleanupManager().push([&](VkDevice dev) {
            vkDestroyPipeline(dev, _pipeline, nullptr);
            vkDestroyPipelineLayout(dev, _layout, nullptr);
        });
    }

	void createVertexData()
	{
		using namespace bg2e::render::vulkan;

		auto bufferSize = sizeof(_vertices[0]) * _vertices.size();
		auto stagingBuffer = std::unique_ptr<Buffer>(Buffer::createAllocatedBuffer(
			_vulkan,
			bufferSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VMA_MEMORY_USAGE_CPU_ONLY
		));

		void* dataPtr = stagingBuffer->allocatedData();

		memcpy(dataPtr, _vertices.data(), bufferSize);

		_vertexBuffer = std::unique_ptr<Buffer>(Buffer::createAllocatedBuffer(
			_vulkan,
			bufferSize,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VMA_MEMORY_USAGE_GPU_ONLY
		));

		auto indexBufferSize = sizeof(_indices[0]) * _indices.size();
		auto indexStagingBuffer = std::unique_ptr<Buffer>(Buffer::createAllocatedBuffer(
			_vulkan,
			indexBufferSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VMA_MEMORY_USAGE_CPU_ONLY
		));

		dataPtr = indexStagingBuffer->allocatedData();
		memcpy(dataPtr, _indices.data(), indexBufferSize);

		_indexBuffer = std::unique_ptr<Buffer>(Buffer::createAllocatedBuffer(
			_vulkan,
			indexBufferSize,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			VMA_MEMORY_USAGE_GPU_ONLY
		));

		_vulkan->command().immediateSubmit([&](VkCommandBuffer cmd) {
			VkBufferCopy copyData = {};
			copyData.dstOffset = 0;
			copyData.srcOffset = 0;
			copyData.size = bufferSize;
			vkCmdCopyBuffer(cmd, stagingBuffer->handle(), _vertexBuffer->handle(), 1, &copyData);

			copyData.size = indexBufferSize;
			vkCmdCopyBuffer(cmd, indexStagingBuffer->handle(), _indexBuffer->handle(), 1, &copyData);
		});

		stagingBuffer->cleanup();
		indexStagingBuffer->cleanup();

		_vulkan->cleanupManager().push([this](VkDevice dev) {
			_vertexBuffer->cleanup();
			_indexBuffer->cleanup();
		});
	}

	void createImage(VkExtent2D extent)
	{
		using namespace bg2e::render::vulkan;
		auto vulkan = this->vulkan();
		_targetImage = std::shared_ptr<Image>(Image::createAllocatedImage(
			vulkan,
			VK_FORMAT_R16G16B16A16_SFLOAT,
			extent,
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
			VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			VK_IMAGE_ASPECT_COLOR_BIT
		));
	}
};

class MyApplication : public bg2e::app::Application {
public:
	void init(int argc, char** argv) override
	{
		auto delegate = std::shared_ptr<ClearScreenDelegate>(new ClearScreenDelegate());
		setRenderDelegate(delegate);
		setInputDelegate(delegate);
		setUiDelegate(delegate);
	}
};

int main(int argc, char ** argv) {

    bg2e::app::MainLoop mainLoop;
	MyApplication app;
	app.init(argc, argv);
    return mainLoop.run(&app);
}
