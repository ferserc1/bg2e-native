
#ifndef _bg2_file_image_load_hpp_
#define _bg2_file_image_load_hpp_

#include <bg2-base/image.hpp>

namespace bg2db {
    
    extern bg2base::image * loadImage(const bg2base::path & filePath);
    extern bg2base::image * loadImage(const std::string & filePath);
    extern bg2base::image * loadImage(const char * filePath);
}

#endif
