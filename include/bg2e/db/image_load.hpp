
#ifndef _bg2e_db_image_load_hpp_
#define _bg2e_db_image_load_hpp_

#include <bg2e/base/image.hpp>

namespace bg2e {
namespace db {

	extern bg2e::base::image* loadImage(const bg2e::base::path& filePath);
	extern bg2e::base::image* loadImage(const std::string& filePath);
	extern bg2e::base::image* loadImage(const char* filePath);

}
}
#endif
