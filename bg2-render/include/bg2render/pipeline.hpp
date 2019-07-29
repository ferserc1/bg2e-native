#ifndef _bg2render_pipeline_hpp_
#define _bg2render_pipeline_hpp_

#include <vulkan/vulkan.h>

#include <functional>
#include <vector>
#include <string>

#include <bg2render/vk_instance.hpp>

namespace bg2render {
    
    class Pipeline {
	public:
		struct ShaderData {
			VkShaderModule module;
			std::string mainFunction;
		};

		Pipeline(vk::Instance * instance);
		virtual ~Pipeline();


		// Shaders
		void addShader(const std::vector<char>& buffer, VkShaderStageFlagBits type, const std::string & mainFunction);
		inline const std::vector<VkPipelineShaderStageCreateInfo>& shaderStages() const { return _shaderStages; }

	protected:
		// Instance
		vk::Instance* _instance;

		// Creation info
		std::vector<ShaderData> _shaderStagesData;
		std::vector<VkPipelineShaderStageCreateInfo> _shaderStages;

    };

}

#endif
