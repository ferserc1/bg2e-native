#ifndef _bg2e_db_texture_load_hpp_
#define _bg2e_db_texture_load_hpp_

#include <bg2e/base/texture.hpp>

#include <string>

namespace bg2e {
namespace db {

	extern bg2e::base::Texture * loadTexture(const bg2e::base::path & filePath);
	extern bg2e::base::Texture * loadTexture(const std::string & filePath);
	extern bg2e::base::Texture * loadTexture(const char * filePath);

}
}

#endif