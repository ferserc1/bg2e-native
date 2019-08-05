#ifndef _bg2render_pipeline_hpp_
#define _bg2render_pipeline_hpp_

#include <vulkan/vulkan.h>

#include <functional>
#include <vector>
#include <string>

#include <bg2math/vector.hpp>
#include <bg2render/vk_instance.hpp>
#include <bg2render/render_pass.hpp>

namespace bg2render {
    
    class Pipeline {
	public:
		struct ShaderData {
			VkShaderModule module;
			std::vector<char> mainFunction;
		};

		Pipeline(vk::Instance * instance);
		virtual ~Pipeline();


		// Shaders
		void addShader(const std::vector<char>& buffer, VkShaderStageFlagBits type, const std::string & mainFunction);
		inline const std::vector<VkPipelineShaderStageCreateInfo>& shaderStages() const { return _shaderStages; }

		// Vertex input
		// TODO: add vertex and attribute descriptions
		inline const VkPipelineVertexInputStateCreateInfo& vertexInputInfo() const { return _vertexInputInfo; }
		inline VkPipelineVertexInputStateCreateInfo& vertexInputInfo() { return _vertexInputInfo; }

		// Input assembly
		inline const VkPipelineInputAssemblyStateCreateInfo& inputAssemblyInfo() const { return _inputAssemblyInfo; }
		inline VkPipelineInputAssemblyStateCreateInfo& inputAssemblyInfo() { return _inputAssemblyInfo; }

		// Viewport and scissor
		inline void setViewport(const bg2math::int2& size, float minDepth = 0.0f, float maxDepth = 1.0f) {
			setViewport(bg2math::int2(0, 0), size, minDepth, maxDepth);
		}
		inline void setViewport(const bg2math::int2& origin, const bg2math::int2& size, float minDepth = 0.0f, float maxDepth = 1.0f) {
			_viewport.x = static_cast<float>(origin.x());
			_viewport.y = static_cast<float>(origin.y());
			_viewport.width = static_cast<float>(size.x());
			_viewport.height = static_cast<float>(size.y());
			_viewport.minDepth = minDepth;
			_viewport.maxDepth = maxDepth;
			if (_scissor.extent.width == 0 && _scissor.extent.height == 0) {
				_scissor.extent.width = size.x();
				_scissor.extent.height = size.y();
			}
		}
		void setScissor(const bg2math::int2& offset, const bg2math::int2& size) {
			_scissor.offset.x = offset.x();
			_scissor.offset.y = offset.y();
			_scissor.extent.width = size.x();
			_scissor.extent.height = size.y();
		}
		inline const VkPipelineViewportStateCreateInfo& viewportState() const { return _viewportState; }

		// Rasterization state
		inline const VkPipelineRasterizationStateCreateInfo& rasterizationStateInfo() const { return _rasterizationStateInfo; }
		inline VkPipelineRasterizationStateCreateInfo& rasterizationStateInfo() { return _rasterizationStateInfo; }

		// Multisampling
		inline const VkPipelineMultisampleStateCreateInfo& multisamplingStateInfo() const { return _multisamplingStateInfo; }
		inline VkPipelineMultisampleStateCreateInfo& multisamplingStateInfo() { return _multisamplingStateInfo; }

		// Color blend
		void addColorBlendAttachment(const VkPipelineColorBlendAttachmentState& att);
		void loadDefaultBlendAttachments();
		inline const VkPipelineColorBlendStateCreateInfo & colorBlendInfo() const { return _colorBlendInfo; }
		inline VkPipelineColorBlendStateCreateInfo & colorBlendInfo() { return _colorBlendInfo; }

		// Dynamic state
		inline void addDynamicState(VkDynamicState s) {
			_dynamicStates.push_back(s);
			_dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(_dynamicStates.size());
			_dynamicStateInfo.pDynamicStates = _dynamicStates.data();
		}
		inline const VkPipelineDynamicStateCreateInfo& dynamicStateInfo() const { return _dynamicStateInfo; }

		// Pipeline layout
		// If you create the layout using this utility class functions, it will be automatically removed
		void createDefaultLayout();	
		// If you use the setPipelineLayout function, you will need to delete the layout
		void setPipelineLayout(VkPipelineLayout lo);
		inline VkPipelineLayout pipelineLayout() const { return _pipelineLayout; }

		// Render pass
		inline void setRenderPass(RenderPass* rp, uint32_t subpass = 0) { _renderPass = std::shared_ptr<RenderPass>(rp); _subpass = subpass; }
		void createDefaultRenderPass(VkFormat format);
		inline const RenderPass* renderPass() const { return _renderPass.get(); }
		inline RenderPass* renderPass() { return _renderPass.get(); }

		// Create pipeline
		void create();

		inline VkPipeline pipeline() const { return _pipeline; }
		
	protected:
		// Instance
		vk::Instance* _instance;

		// Creation info
		std::vector<ShaderData> _shaderStagesData;
		std::vector<VkPipelineShaderStageCreateInfo> _shaderStages;
		VkPipelineVertexInputStateCreateInfo _vertexInputInfo = {
			VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
			nullptr,
			0,
			0,
			nullptr,
			0,
			nullptr
		};
		VkPipelineInputAssemblyStateCreateInfo _inputAssemblyInfo = {
		   VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		   nullptr,
		   0,
		   VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		   VK_FALSE
		};
		VkViewport _viewport = {};
		VkRect2D _scissor = {};
		VkPipelineViewportStateCreateInfo _viewportState = {
			VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
			nullptr,
			0,
			1,
			&_viewport,
			1,
			&_scissor
		};
		VkPipelineRasterizationStateCreateInfo _rasterizationStateInfo{
			VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
			nullptr,
			0,
			VK_FALSE,	// depthClampEnable
			VK_FALSE,	// rasterizerDiscardEnable
			VK_POLYGON_MODE_FILL,
			VK_CULL_MODE_BACK_BIT,
			VK_FRONT_FACE_CLOCKWISE,
			VK_FALSE,	// depthBias enabled
			0.0f,	// depthBiasConstantFactor;
			0.0f,   // depthBiasClamp;
			0.0f,	// depthBiasSlopeFactor;
			1.0f	// lineWidth;
		};
		VkPipelineMultisampleStateCreateInfo _multisamplingStateInfo = {
			VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
			nullptr,
			0,
			VK_SAMPLE_COUNT_1_BIT,
			VK_FALSE,
			1.0f,
			nullptr,
			VK_FALSE,
			VK_FALSE
		};
		// TODO: depthStencil info default values
		VkPipelineDepthStencilStateCreateInfo _depthStencilInfo = {
		};
		// Color blend
		std::vector<VkPipelineColorBlendAttachmentState> _colorBlendAttachments;
		VkPipelineColorBlendStateCreateInfo _colorBlendInfo = {
			VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
			nullptr,
			0,
			VK_FALSE,
			VK_LOGIC_OP_COPY,
			0,
			nullptr,
			{ 0.0f, 0.0f, 0.0f, 0.0f }
		};
		// Dynamic state
		std::vector<VkDynamicState> _dynamicStates;
		VkPipelineDynamicStateCreateInfo _dynamicStateInfo = {
			VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
			nullptr,
			0,
			0,
			nullptr
		};

		// Pipeline layout
		VkPipelineLayout _pipelineLayout = VK_NULL_HANDLE;
		bool _destroyLayout = false;

		// Render pass
		std::shared_ptr<RenderPass> _renderPass;
		uint32_t _subpass;

		// Pipeline
		VkPipeline _pipeline = VK_NULL_HANDLE;
    };

}

#endif
