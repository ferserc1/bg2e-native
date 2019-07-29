

#include <bg2wnd/application.hpp>
#include <bg2wnd/window.hpp>
#include <bg2wnd/window_delegate.hpp>
#include <bg2render/vk_instance.hpp>
#include <bg2render/vk_device.hpp>
#include <bg2render/swap_chain.hpp>
#include <bg2math/vector.hpp>
#include <bg2db/buffer_load.hpp>



#include <iostream>

#include <vulkan/vulkan.h>

#include <string.h>



class MyDelegate : public bg2wnd::WindowDelegate {
public:
    MyDelegate(bool enableValidation) :_enableValidationLayers(enableValidation) {}

	int _maxFramesInFlight;

	VkShaderModule _vertShaderModule;
	VkShaderModule _fragShaderModule;
	VkPipelineLayout _pipelineLayout;
	VkRenderPass _renderPass;
	VkPipeline _graphicsPipeline;
	VkCommandPool _commandPool;
	std::vector<VkCommandBuffer> _commandBuffers;

	size_t _currentFrame = 0;
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
		VkGraphicsPipelineCreateInfo pipelineInfo = {};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

		// SHADERS
		bg2base::path shaderPath = "shaders";
		auto vshader = bg2db::loadBuffer(shaderPath.pathAddingComponent("basic.vert.spv"));
		auto fshader = bg2db::loadBuffer(shaderPath.pathAddingComponent("basic.frag.spv"));
		VkShaderModuleCreateInfo shaderCreateInfo = {};
		shaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		shaderCreateInfo.codeSize = vshader.size();
		shaderCreateInfo.pCode = reinterpret_cast<const uint32_t*>(vshader.data());
		if (vkCreateShaderModule(_instance->renderDevice()->device(), &shaderCreateInfo, nullptr, &_vertShaderModule) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create vertex shader module");
		}

		shaderCreateInfo.codeSize = fshader.size();
		shaderCreateInfo.pCode = reinterpret_cast<const uint32_t*>(fshader.data());
		if (vkCreateShaderModule(_instance->renderDevice()->device(), &shaderCreateInfo, nullptr, &_fragShaderModule) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create fragment shader module");
		}

		VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = _vertShaderModule;
		vertShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = _fragShaderModule;
		fragShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;


		// Vertex input
		VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 0;
		vertexInputInfo.pVertexBindingDescriptions = nullptr;
		vertexInputInfo.vertexAttributeDescriptionCount = 0;
		vertexInputInfo.pVertexAttributeDescriptions = nullptr;

		pipelineInfo.pVertexInputState = &vertexInputInfo;

		// Input Assembly
		VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		pipelineInfo.pInputAssemblyState = &inputAssembly;

		// Viewport and scissor
		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(window()->size().x());
		viewport.height = static_cast<float>(window()->size().y());
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor = {};
		scissor.offset = { 0, 0 };
		scissor.extent.width = window()->size().x();
		scissor.extent.height = window()->size().y();

		VkPipelineViewportStateCreateInfo viewportState = {};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.pViewports = &viewport;
		viewportState.scissorCount = 1;
		viewportState.pScissors = &scissor;

		pipelineInfo.pViewportState = &viewportState;

		// Rasterizer
		VkPipelineRasterizationStateCreateInfo rasterizer = {};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizer.lineWidth = 1.0f;
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;
		rasterizer.depthBiasConstantFactor = 0.0f;
		rasterizer.depthBiasClamp = 0.0f;
		rasterizer.depthBiasSlopeFactor = 0.0f;

		pipelineInfo.pRasterizationState = &rasterizer;

		// Multisampling
		VkPipelineMultisampleStateCreateInfo multisampling = {};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampling.minSampleShading = 1.0f;
		multisampling.pSampleMask = nullptr;
		multisampling.alphaToCoverageEnable = VK_FALSE;
		multisampling.alphaToOneEnable = VK_FALSE;

		pipelineInfo.pMultisampleState = &multisampling;

		// Depth and stencil test
		VkPipelineDepthStencilStateCreateInfo depthStencil = {};
		// TODO: configure depth test
		pipelineInfo.pDepthStencilState = nullptr;

		// Color blend
		VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
		colorBlendAttachment.colorWriteMask =
			VK_COLOR_COMPONENT_R_BIT |
			VK_COLOR_COMPONENT_G_BIT |
			VK_COLOR_COMPONENT_B_BIT |
			VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_FALSE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

		VkPipelineColorBlendStateCreateInfo colorBlending = {};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY;
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;
		colorBlending.blendConstants[0] = 0.0f;
		colorBlending.blendConstants[1] = 0.0f;
		colorBlending.blendConstants[2] = 0.0f;
		colorBlending.blendConstants[3] = 0.0f;

		pipelineInfo.pColorBlendState = &colorBlending;

		// Dynamic state
		VkDynamicState dynamicStates[] = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_LINE_WIDTH
		};

		VkPipelineDynamicStateCreateInfo dynamicState = {};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = 2;
		dynamicState.pDynamicStates = dynamicStates;

		// TODO: Use dynamic state
		pipelineInfo.pDynamicState = nullptr;

		VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 0;
		pipelineLayoutInfo.pSetLayouts = nullptr;
		pipelineLayoutInfo.pushConstantRangeCount = 0;
		pipelineLayoutInfo.pPushConstantRanges = nullptr;
		if (vkCreatePipelineLayout(_instance->renderDevice()->device(), &pipelineLayoutInfo, nullptr, &_pipelineLayout) != VK_SUCCESS) {
			throw std::runtime_error("Error creating pipeline layout");
		}

		pipelineInfo.layout = _pipelineLayout;

		///// Render passes
		VkAttachmentDescription colorAttachment = {};
		colorAttachment.format = _swapChain->format();
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		// Subpasses and attachment references
		VkAttachmentReference colorAttachmentRef = {};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;
		
		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = 1;
		renderPassInfo.pAttachments = &colorAttachment;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;

		// Subpass dependencies
		VkSubpassDependency dependency = {};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask =
			VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		if (vkCreateRenderPass(_instance->renderDevice()->device(), &renderPassInfo, nullptr, &_renderPass) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create render pass");
		}

		// Create framebuffers
		_swapChain->createFramebuffers(_renderPass);

		pipelineInfo.renderPass = _renderPass;
		pipelineInfo.subpass = 0;

		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
		pipelineInfo.basePipelineIndex = -1;

		////// Create pipeline
		if (vkCreateGraphicsPipelines(_instance->renderDevice()->device(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_graphicsPipeline) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create graphics pipeline");
		}

		

		/////// Command pool and command buffers
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
		_maxFramesInFlight = _swapChain->images().size();
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

    void resize(const bg2math::int2 & size) {
        std::cout << "resize: " << size.x() << ", " << size.y() << std::endl;
    }

    void update(float delta) {
		vkWaitForFences(_instance->renderDevice()->device(), 1, &_inFlightFences[_currentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());
		vkResetFences(_instance->renderDevice()->device(), 1, &_inFlightFences[_currentFrame]);

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
		renderPassInfo.renderPass = _renderPass;
		renderPassInfo.framebuffer = _swapChain->framebuffers()[i];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = _swapChain->extent();
		VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;
		vkCmdBeginRenderPass(_commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, _graphicsPipeline);
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
		vkAcquireNextImageKHR(
			_instance->renderDevice()->device(),
			_swapChain->swapchain(),
			std::numeric_limits<uint64_t>::max(),
			_imageAvailableSemaphore[_currentFrame],
			VK_NULL_HANDLE,
			&imageIndex);

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

		for (size_t i = 0; i < _maxFramesInFlight; ++i) {
			vkDestroySemaphore(_instance->renderDevice()->device(), _imageAvailableSemaphore[i], nullptr);
			vkDestroySemaphore(_instance->renderDevice()->device(), _renderFinishedSemaphore[i], nullptr);
			vkDestroyFence(_instance->renderDevice()->device(), _inFlightFences[i], nullptr);
		}

		vkDestroyCommandPool(_instance->renderDevice()->device(), _commandPool, nullptr);
		vkDestroyPipeline(_instance->renderDevice()->device(), _graphicsPipeline, nullptr);
		vkDestroyPipelineLayout(_instance->renderDevice()->device(), _pipelineLayout, nullptr);
		vkDestroyRenderPass(_instance->renderDevice()->device(), _renderPass, nullptr);
		vkDestroyShaderModule(_instance->renderDevice()->device(), _fragShaderModule, nullptr);
		vkDestroyShaderModule(_instance->renderDevice()->device(), _vertShaderModule, nullptr);

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
