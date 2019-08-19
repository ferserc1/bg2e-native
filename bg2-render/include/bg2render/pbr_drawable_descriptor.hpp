
#ifndef _bg2render_pbr_drawable_descriptor_hpp_
#define _bg2render_pbr_drawable_descriptor_hpp_

#include <bg2render/drawable_descriptor.hpp>

namespace bg2render {

	class PBRDrawableDescriptor : public DrawableDescriptor {
	protected:
		virtual void configureLayout(vk::PipelineLayout*);
		virtual void configureDescriptorPool(vk::DescriptorPool*, uint32_t poolSize);
		virtual void updateDescriptorWrites(vk::Instance*, uint32_t, DrawableItem*);
	};

}

#endif
