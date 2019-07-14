
#ifndef _bg2_db_image_write_hpp_
#define _bg2_db_image_write_hpp_

#include <bg2base/image.hpp>
#include <bg2base/path.hpp>

namespace bg2db {
    
    extern void writeImage(const bg2base::path & dest, const bg2base::image & img);
    extern void writeImage(const std::string & dest, const bg2base::image & img);
    extern void writeImage(const char * dest, const bg2base::image & img);
    extern void writeImage(const bg2base::path & dest, const bg2base::image * img);
    extern void writeImage(const std::string & dest, const bg2base::image * img);
    extern void writeImage(const char * dest, const bg2base::image * img);
}

#endif
