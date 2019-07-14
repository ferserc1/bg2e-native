
#ifndef _bg2_base_image_hpp_
#define _bg2_base_image_hpp_

#include <bg2-base/path.hpp>

namespace bg2base {
    
    class Image {
    public:
        Image();
        Image(const std::string & path);
        Image(const char * path);
        Image(const path & path);
        
        inline const path & filePath() const { return _path; }
        
    protected:
        path _path;
    };
    
}

#endif
