
#include <bg2-base/image.hpp>

namespace bg2base {
    
    Image::Image()
    {
        
    }
    
    Image::Image(const std::string & path)
        :_path(path)
    {
        
    }
    
    Image::Image(const char * path)
        :_path(path)
    {
        
    }
    
    Image::Image(const path & path)
        :_path(path)
    {
        
    }
}
