

#include <bg2wnd/application.hpp>
#include <bg2wnd/window.hpp>
#include <bg2wnd/window_delegate.hpp>
#include <bg2render/vk_instance.hpp>
#include <bg2render/vk_device.hpp>
#include <bg2render/swap_chain.hpp>
#include <bg2math/vector.hpp>
#include <bg2db/buffer_load.hpp>
#include <bg2render/pipeline.hpp>



#include <iostream>

#include <vulkan/vulkan.h>

#include <string.h>



class MyDelegate : public bg2wnd::WindowDelegate {
public:
    MyDelegate(bool enableValidation) :_enableValidationLayers(enableValidation) {}

	int _maxFramesInFlight;

	std::unique_ptr<bg2render::Pipeline> _pipeline;

	//VkPipeline _graphicsPipeline;
	VkCommandPool _commandPool;
	std::vector<VkCommandBuffer> _commandBuffers;

	size_t _currentFrame = 0;
	bool _windowResized = false;
	std::vector<VkSemaphore> _imageAvailableSemaphore;
	std::vector<VkSemaphore> _renderFinishedSemaphore;
	std::vector<VkFence> _inFlightFences;


	void init() {
		_instance = std::unique_ptr<bg2render::vk::Instance>(new bg2render::vk::Instance());
		_instance->configureAppName("bg2e vulkan test");
		if (_enableValidationLayers) {
			_instance->setDebugCallback([](
				VkDebugUtilsMessageSeverityFlagBitsEXT severity,
				VkDebugUtilsMessageTypeFlagsEXT type,
				const VkDebugUtilsMessengerCallbackDataEXT* pData) {
					if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
						std::cout << pData->pMessage << std::endl;
					}
				});
		}

		std::vector<VkExtensionProperties> extensionData;
		_instance->enumerateInstanceExtensionProperties(extensionData);
		std::cout << "Available extensions:" << std::endl;
		for (const auto& ext : extensionData) {
			std::cout << "\t" << ext.extensionName << std::endl;
		}

		std::vector<const char*> extensions;
		window()->getVulkanRequiredInstanceExtensions(extensions);
		_instance->configureRequiredExtensions(extensions);

		_instance->create();

		// It's important to link the window surface to vulkan instance BEFORE
		// choose physical device, if you want to get support for presentation queues
		_instance->setSurface(window()->createVulkanSurface(_instance->instance()));

		_instance->choosePhysicalDevices();

		auto queue = _instance->renderQueue();
		auto presentQueue = _instance->presentQueue();

		_swapChain = std::unique_ptr<bg2render::SwapChain>(new bg2render::SwapChain(
			_instance->renderPhysicalDevice(),
			_instance->renderDevice(),
			_instance->surface()));
		_swapChain->create(window()->size());


		///// Pipeline
		_pipeline = std::make_unique<bg2render::Pipeline>(_instance.get());

		//VkGraphicsPipelineCreateInfo pipelineInfo = {};
		//pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

		// SHADERS
		bg2base::path shaderPath = "shaders";
		auto vshader = bg2db::loadBuffer(shaderPath.pathAddingComponent("basic.vert.spv"));
		auto fshader = bg2db::loadBuffer(shaderPath.pathAddingComponent("basic.frag.spv"));
		_pipeline->addShader(vshader, VK_SHADER_STAGE_VERTEX_BIT, "main");
		_pipeline->addShader(fshader, VK_SHADER_STAGE_FRAGMENT_BIT, "main");

		// TODO: Vertex input
		
		// Input Assembly		
		_pipeline->inputAssemblyInfo().topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		
		// Viewport and scissor
		_pipeline->setViewport(window()->size());
		
		// Rasterizer
		_pipeline->rasterizationStateInfo().cullMode = VK_CULL_MODE_BACK_BIT;
		_pipeline->rasterizationStateInfo().frontFace = VK_FRONT_FACE_CLOCKWISE;
		
		// Depth and stencil test
		// TODO: configure depth test
		
		// Color blend
		_pipeline->loadDefaultBlendAttachments();
		
		// Dynamic state
		// TODO: Use dynamic state
	//	_pipeline->addDynamicState(VK_DYNAMIC_STATE_VIEWPORT);
	//	_pipeline->addDynamicState(VK_DYNAMIC_STATE_LINE_WIDTH);
		
		_pipeline->createDefaultLayout();
		
		///// Render passes
		_pipeline->createDefaultRenderPass(_swapChain->format());

		// Create framebuffers
		_swapChain->createFramebuffers(_pipeline->renderPass());

		////// Create pipeline
		_pipeline->create();

		/////// Command pool and command buffers
		createCommandBuffers();

		
		
    }

	void createCommandBuffers() {
		VkCommandPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.queueFamilyIndex = _instance->renderPhysicalDevice()->queueIndices().graphicsFamily;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		//poolInfo.flags = 0;

		if (vkCreateCommandPool(_instance->renderDevice()->device(), &poolInfo, nullptr, &_commandPool) != VK_SUCCESS) {
			throw std::runtime_error("Error creating command pool");
		}

		// One command buffer for each framebuffer
		_commandBuffers.resize(_swapChain->framebuffers().size());
		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = _commandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = static_cast<uint32_t>(_commandBuffers.size());
		if (vkAllocateCommandBuffers(_instance->renderDevice()->device(), &allocInfo, _commandBuffers.data()) != VK_SUCCESS) {
			throw std::runtime_error("Error allocating command buffers");
		}

		////// CPU-GPU rendering sinchronization
		_maxFramesInFlight = static_cast<uint32_t>(_swapChain->images().size());
		_imageAvailableSemaphore.resize(_maxFramesInFlight);
		_renderFinishedSemaphore.resize(_maxFramesInFlight);
		_inFlightFences.resize(_maxFramesInFlight);

		VkSemaphoreCreateInfo semaphoreInfo = {};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo = {};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (size_t i = 0; i < _maxFramesInFlight; ++i) {
			if (vkCreateSemaphore(_instance->renderDevice()->device(), &semaphoreInfo, nullptr, &_imageAvailableSemaphore[i]) != VK_SUCCESS ||
				vkCreateSemaphore(_instance->renderDevice()->device(), &semaphoreInfo, nullptr, &_renderFinishedSemaphore[i]) != VK_SUCCESS ||
				vkCreateFence(_instance->renderDevice()->device(), &fenceInfo, nullptr, &_inFlightFences[i]) != VK_SUCCESS) {
				throw std::runtime_error("Failed to create sinchronization objects");
			}
		}
	}

	void destroyCommandBuffers() {
		vkFreeCommandBuffers(_instance->renderDevice()->device(), _commandPool, static_cast<uint32_t>(_commandBuffers.size()), _commandBuffers.data());
		_commandBuffers.clear();
		vkDestroyCommandPool(_instance->renderDevice()->device(), _commandPool, nullptr);

		for (size_t i = 0; i < _maxFramesInFlight; ++i) {
			vkDestroySemaphore(_instance->renderDevice()->device(), _imageAvailableSemaphore[i], nullptr);
			vkDestroySemaphore(_instance->renderDevice()->device(), _renderFinishedSemaphore[i], nullptr);
			vkDestroyFence(_instance->renderDevice()->device(), _inFlightFences[i], nullptr);
		}
	}

    void resize(const bg2math::int2 & size) {
        std::cout << "resize: " << size.x() << ", " << size.y() << std::endl;
		_windowResized = true;
    }

    void update(float delta) {
		vkWaitForFences(_instance->renderDevice()->device(), 1, &_inFlightFences[_currentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());

		// Start recording commands
		size_t i = _currentFrame;
		vkResetCommandBuffer(_commandBuffers[i], 0);
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = 0;
		beginInfo.pInheritanceInfo = nullptr;
		if (vkBeginCommandBuffer(_commandBuffers[i], &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("Failed to begin recording command buffer");
		}

		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = _pipeline->renderPass()->renderPass();// _renderPass;
		renderPassInfo.framebuffer = _swapChain->framebuffers()[i];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = _swapChain->extent();
		VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;
		vkCmdBeginRenderPass(_commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline->pipeline());// _graphicsPipeline);
		vkCmdDraw(_commandBuffers[i],
			3,	// Vertex count 
			1,	// Instance count
			0,	// First vertex
			0);	// First instance

		vkCmdEndRenderPass(_commandBuffers[i]);

		if (vkEndCommandBuffer(_commandBuffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("Failed to record command buffer");
		}
    }

    void draw() {
		

		uint32_t imageIndex;
		VkResult result= vkAcquireNextImageKHR(
			_instance->renderDevice()->device(),
			_swapChain->swapchain(),
			std::numeric_limits<uint64_t>::max(),
			_imageAvailableSemaphore[_currentFrame],
			VK_NULL_HANDLE,
			&imageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR || _windowResized) {
			bg2math::int2 size = window()->size();
			vkDeviceWaitIdle(_instance->renderDevice()->device());
			vkResetCommandPool(_instance->renderDevice()->device(), _commandPool, 0);
			destroyCommandBuffers();
			_swapChain->resize(size, _pipeline->renderPass()->renderPass());
			_pipeline->resize(size);
			createCommandBuffers();
			_currentFrame = 0;
			_windowResized = false;
			return;
		}
		else if (result != VK_SUCCESS) {
			throw std::runtime_error("Failed to acquire swap chain image");
		}

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphore waitSemaphores[] = { _imageAvailableSemaphore[_currentFrame] };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &_commandBuffers[imageIndex];
		
		VkSemaphore signalSemaphores[] = { _renderFinishedSemaphore[_currentFrame] };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		vkResetFences(_instance->renderDevice()->device(), 1, &_inFlightFences[_currentFrame]);

		if (vkQueueSubmit(_instance->renderQueue(), 1, &submitInfo, _inFlightFences[_currentFrame]) != VK_SUCCESS) {
			throw std::runtime_error("Failed to submit draw command buffer");
		}

		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;

		VkSwapchainKHR swapchains[] = { _swapChain->swapchain() };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapchains;
		presentInfo.pImageIndices = &imageIndex;
		presentInfo.pResults = nullptr;
		vkQueuePresentKHR(_instance->presentQueue(), &presentInfo);

		_currentFrame = (_currentFrame + 1) % _maxFramesInFlight;
    }

    void cleanup() {
		vkDeviceWaitIdle(_instance->renderDevice()->device());

		destroyCommandBuffers();

		_pipeline = nullptr;
		_swapChain = nullptr;
		_instance = nullptr;
    }

    void keyUp(const bg2wnd::KeyboardEvent & e) {}
    void keyDown(const bg2wnd::KeyboardEvent & e) {}
    void mouseMove(const bg2wnd::MouseEvent & e) {}
    void mouseDown(const bg2wnd::MouseEvent & e) {}
    void mouseUp(const bg2wnd::MouseEvent & e) {}
    void mouseWheel(const bg2wnd::MouseEvent & e) {}

private:
	std::unique_ptr<bg2render::vk::Instance> _instance;
	std::unique_ptr<bg2render::SwapChain> _swapChain;

	bool _enableValidationLayers;
};

int main(int argc, char ** argv) {
    bg2wnd::Application app;

    auto window = bg2wnd::Window::Create();
    window->setWindowDelegate(new MyDelegate(true));
    app.addWindow(window);
    app.run();

    return 0;
}
