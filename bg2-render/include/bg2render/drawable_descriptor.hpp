
#ifndef _bg2render_drawable_descriptor_hpp_
#define _bg2render_drawable_descriptor_hpp_

#include <bg2render/vk_descriptor_pool.hpp>
#include <bg2render/vk_descriptor_set.hpp>
#include <bg2base/export.hpp>

#include <stdexcept>
#include <map>

namespace bg2render {

	struct Descriptor {
		std::shared_ptr<bg2render::vk::DescriptorPool> descriptorPool;
		std::vector<std::shared_ptr<bg2render::vk::DescriptorSet>> descriptorSets;
	};

	class DrawableItem;
	class DrawableDescriptor {
	public:
		template <class T>
		static void Register(const std::string& identifier) {
			s_drawableDescriptors[identifier] = std::make_unique<T>();
		}

		static DrawableDescriptor* Get(const std::string& identifier) {
			if (s_drawableDescriptors.find(identifier) == s_drawableDescriptors.end()) {
				throw std::invalid_argument("No such DrawableDescriptor with identifier " + identifier);
			}
			return s_drawableDescriptors[identifier].get();
		}

		static void Cleanup() {
			s_drawableDescriptors.clear();
		}

		DrawableDescriptor() {}
		virtual ~DrawableDescriptor() {
		}

		bg2render::vk::PipelineLayout* createPipelineLayout(vk::Instance* instance);

		Descriptor* createDescriptorPool(vk::Instance* instance, vk::PipelineLayout* pipelineLayout, uint32_t poolSize);

		virtual void updateDescriptorWrites(vk::Instance* instance, uint32_t frameIndex, DrawableItem*) = 0;

	protected:
		virtual void configureLayout(bg2render::vk::PipelineLayout* pipelineLayout) = 0;
		virtual void configureDescriptorPool(bg2render::vk::DescriptorPool* descriptorPool, uint32_t poolSize) = 0;

		static EXPORT std::map<std::string, std::unique_ptr<DrawableDescriptor>> s_drawableDescriptors;
	};

	

	template <class T>
	class DrawableDescriptorRegistry {
	public:
		DrawableDescriptorRegistry(const std::string& identifier) {
			DrawableDescriptor::Register<T>(identifier);
		}
	};

}

#endif
