
#include <bg2render/renderer.hpp>
#include <bg2render/pipeline.hpp>

#include <bg2base/path.hpp>
#include <bg2db/buffer_load.hpp>

#include <stdexcept>

namespace bg2render {

	Renderer::Renderer(vk::Instance* instance)
		:_instance(instance)
	{
		
	}

	Renderer::~Renderer() {
		vkDeviceWaitIdle(_instance->renderDevice()->device());

		freeRenderingObjects();

		_pipeline = nullptr;
		_swapChain = nullptr;
	}

	void Renderer::init(const bg2math::int2& frameSize) {
		_swapChain = std::unique_ptr<bg2render::SwapChain>(new bg2render::SwapChain(
			_instance->renderPhysicalDevice(),
			_instance->renderDevice(),
			_instance->surface()
		));
		_swapChain->create(frameSize);

		createPipeline(frameSize);
		
		configureRenderingObjects();
	}

	void Renderer::resize(const bg2math::int2& frameSize) {
		if (_instance != nullptr) {
			vkDeviceWaitIdle(_instance->renderDevice()->device());

			vkResetCommandPool(_instance->renderDevice()->device(), _commandPool, 0);
			freeRenderingObjects();

			_swapChain->resize(frameSize, _pipeline->renderPass());
			_pipeline->resize(frameSize);
			configureRenderingObjects();

			_currentFrame = 0;
		}
	}

	void Renderer::update(float delta) {
		vkWaitForFences(_instance->renderDevice()->device(), 1, &_inFlightFences[_currentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());

		// Start recording commands
		// TODO: Delegate this to a renderer delegate
		auto commandBuffer = _commandBuffers[_currentFrame];

		vkResetCommandBuffer(commandBuffer, 0);
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("Failed to begin recording command buffer");
		}

		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = _pipeline->renderPass()->renderPass();
		renderPassInfo.framebuffer = _swapChain->framebuffers()[_currentFrame];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = _swapChain->extent();
		VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;
		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline->pipeline());
		vkCmdDraw(commandBuffer,
			3,	// vertex count
			1,	// instance count
			0,	// first vertex
			0);	// first instance

		vkCmdEndRenderPass(commandBuffer);

		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
			throw std::runtime_error("Failed to record command buffer");
		}
	}

	void Renderer::draw() {
		uint32_t imageIndex;
		VkResult result = vkAcquireNextImageKHR(
			_instance->renderDevice()->device(),
			_swapChain->swapchain(),
			std::numeric_limits<uint64_t>::max(),
			_imageAvailableSemaphore[_currentFrame],
			VK_NULL_HANDLE,
			&imageIndex
		);

		if (result != VK_ERROR_OUT_OF_DATE_KHR && result != VK_SUBOPTIMAL_KHR && result != VK_SUCCESS) {
			throw std::runtime_error("Failed to acquire swap chain image");
		}
		else if (result == VK_SUCCESS) {

			///// Submit command buffer to GPU
			VkSubmitInfo submitInfo = {};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

			VkSemaphore waitSemaphores[] = { _imageAvailableSemaphore[_currentFrame] };
			VkSemaphore signalSemaphores[] = { _renderFinishedSemaphore[_currentFrame] };
			VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
			submitInfo.waitSemaphoreCount = 1;
			submitInfo.pWaitSemaphores = waitSemaphores;
			submitInfo.pWaitDstStageMask = waitStages;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &_commandBuffers[_currentFrame];
			submitInfo.signalSemaphoreCount = 1;
			submitInfo.pSignalSemaphores = signalSemaphores;

			vkResetFences(_instance->renderDevice()->device(), 1, &_inFlightFences[_currentFrame]);
			if (vkQueueSubmit(_instance->renderQueue(), 1, &submitInfo, _inFlightFences[_currentFrame]) != VK_SUCCESS) {
				throw std::runtime_error("Failed to submit draw command buffer");
			}

			// Present swapchain image
			VkSwapchainKHR swapchains[] = { _swapChain->swapchain() };
			VkPresentInfoKHR presentInfo = {};
			presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
			presentInfo.waitSemaphoreCount = 1;
			presentInfo.pWaitSemaphores = signalSemaphores;	// Wait until the queue submit has finished
			presentInfo.swapchainCount = 1;
			presentInfo.pSwapchains = swapchains;
			presentInfo.pImageIndices = &imageIndex;
			presentInfo.pResults = nullptr;
			vkQueuePresentKHR(_instance->presentQueue(), &presentInfo);

			// Next frame index
			_currentFrame = (_currentFrame + 1) % _maxInFlightFrames;
		}
		// Otherwise: result == VK_ERROR_OUT_OF_DATE_KHR or VK_SUBOPTIMAL_KHR. The window being resized, cancel render
	}

	void Renderer::configureRenderingObjects() {
		// Command pool
		VkCommandPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.queueFamilyIndex = _instance->renderPhysicalDevice()->queueIndices().graphicsFamily;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		if (vkCreateCommandPool(_instance->renderDevice()->device(), &poolInfo, nullptr, &_commandPool) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create command pool");
		}

		// Allocate one command buffer for each framebuffer
		_commandBuffers.resize(_swapChain->framebuffers().size());
		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = _commandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = static_cast<uint32_t>(_commandBuffers.size());
		if (vkAllocateCommandBuffers(_instance->renderDevice()->device(), &allocInfo, _commandBuffers.data()) != VK_SUCCESS) {
			throw std::runtime_error("Failed to allocate command buffers");
		}

		// CPU-GPU synchronization
		_maxInFlightFrames = static_cast<uint32_t>(_swapChain->images().size());
		_imageAvailableSemaphore.resize(_maxInFlightFrames);
		_renderFinishedSemaphore.resize(_maxInFlightFrames);
		_inFlightFences.resize(_maxInFlightFrames);

		VkSemaphoreCreateInfo semInfo = {};
		semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo = {};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (size_t i = 0; i < _maxInFlightFrames; ++i) {
			if (vkCreateSemaphore(_instance->renderDevice()->device(), &semInfo, nullptr, &_imageAvailableSemaphore[i]) != VK_SUCCESS ||
				vkCreateSemaphore(_instance->renderDevice()->device(), &semInfo, nullptr, &_renderFinishedSemaphore[i]) != VK_SUCCESS ||
				vkCreateFence(_instance->renderDevice()->device(), &fenceInfo, nullptr, &_inFlightFences[i]) != VK_SUCCESS)
			{
				throw std::runtime_error("Failed to create synchronization objects");
			}
		}
	}

	void Renderer::freeRenderingObjects() {
		vkFreeCommandBuffers(_instance->renderDevice()->device(), _commandPool, static_cast<uint32_t>(_commandBuffers.size()), _commandBuffers.data());
		_commandBuffers.clear();
		vkDestroyCommandPool(_instance->renderDevice()->device(), _commandPool, nullptr);

		for (size_t i = 0; i < _maxInFlightFrames; ++i) {
			vkDestroySemaphore(_instance->renderDevice()->device(), _imageAvailableSemaphore[i], nullptr);
			vkDestroySemaphore(_instance->renderDevice()->device(), _renderFinishedSemaphore[i], nullptr);
			vkDestroyFence(_instance->renderDevice()->device(), _inFlightFences[i], nullptr);
		}
	}

	void Renderer::createPipeline(const bg2math::int2& frameSize) {
		// TODO: Delegate pipeline creation in a RendererDelegate
		// Pipeline
		_pipeline = std::make_unique<bg2render::Pipeline>(_instance);


		bg2base::path shaderPath = "shaders";
		auto vshader = bg2db::loadBuffer(shaderPath.pathAddingComponent("basic.vert.spv"));
		auto fshader = bg2db::loadBuffer(shaderPath.pathAddingComponent("basic.frag.spv"));
		_pipeline->addShader(vshader, VK_SHADER_STAGE_VERTEX_BIT, "main");
		_pipeline->addShader(fshader, VK_SHADER_STAGE_FRAGMENT_BIT, "main");

		// TODO: vertex input

		_pipeline->inputAssemblyInfo().topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

		_pipeline->setViewport(frameSize);

		_pipeline->rasterizationStateInfo().cullMode = VK_CULL_MODE_BACK_BIT;
		_pipeline->rasterizationStateInfo().frontFace = VK_FRONT_FACE_CLOCKWISE;

		_pipeline->loadDefaultBlendAttachments();

		_pipeline->createDefaultLayout();

		_pipeline->createDefaultRenderPass(_swapChain->format());

		_swapChain->createFramebuffers(_pipeline->renderPass());

		_pipeline->create();
	}
}
