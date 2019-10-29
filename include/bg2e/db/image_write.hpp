
#ifndef _bg2e_db_image_write_hpp_
#define _bg2e_db_image_write_hpp_

#include <bg2e/base/image.hpp>
#include <bg2e/base/path.hpp>

namespace bg2e {
namespace db {

	extern void writeImage(const bg2e::base::path& dest, const bg2e::base::image& img);
	extern void writeImage(const std::string& dest, const bg2e::base::image& img);
	extern void writeImage(const char* dest, const bg2e::base::image& img);

	extern void writeImage(const bg2e::base::path& dest, const bg2e::base::image* img);
	extern void writeImage(const std::string& dest, const bg2e::base::image* img);
	extern void writeImage(const char* dest, const bg2e::base::image* img);

}
}

#endif
