#pragma once

#include <bg2e/base/Image.hpp>

#include <memory>

namespace bg2e {
namespace base {

class Texture {
public:
    Texture() {}

    // TODO: Add sampler options to constructor parameters
	Texture(Image* image) : _image(std::shared_ptr<Image>(image)) {}
	Texture(std::shared_ptr<Image> image) : _image(image) {}

	inline Image* image() { return _image.get(); }
	inline const Image* image() const { return _image.get(); }

	void setImage(Image* image) { _image = std::shared_ptr<Image>(image); }
    void setImage(std::shared_ptr<Image> image) { _image = image; }

    // TODO: Add sampler options
    
protected:
    std::shared_ptr<Image> _image;

    // TODO: Add sampler options
};

}
}
