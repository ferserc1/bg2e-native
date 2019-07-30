#ifndef _bg2render_pipeline_hpp_
#define _bg2render_pipeline_hpp_

#include <vulkan/vulkan.h>

#include <functional>
#include <vector>
#include <string>

#include <bg2math/vector.hpp>
#include <bg2render/vk_instance.hpp>

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
    };

}

#endif
