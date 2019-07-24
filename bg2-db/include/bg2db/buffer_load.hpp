
#ifndef _bg2db_buffer_load_hpp_
#define _bg2db_buffer_load_hpp_

#include <vector>

#include <bg2base/path.hpp>

namespace bg2db {

    extern std::vector<char> loadBuffer(const bg2base::path & filePath);

}
#endif
