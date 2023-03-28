#ifndef bg2e_base_image_hpp
#define bg2e_base_image_hpp

#include <bg2e/export.hpp>
#include <bg2e/types.hpp>

#include <memory>
#include <string>
#include <unordered_map>

namespace bg2e {
namespace base {

enum class ImageComponentFormat
{
    RGB,
    RGBA,
    BGR,
    BGRA,
    ABGR
};

class BG2E_EXPORT Image {
public:
    static void ClearCache();
    
    static void ClearCacheFile(const std::string& file);
    
    static std::shared_ptr<Image> LoadFromFile(const char* fileName);
    
    static std::shared_ptr<Image> LoadFromFile(const std::string& fileName);
    
    Image();
    ~Image();
    
    inline void set(Byte* imageData, const Size& size, ImageComponentFormat fmt)
    {
        destroy();
        _imageData = imageData;
        _size = size;
        _format = fmt;
        _imageSource = ImageSource::Setter;
    }
    
    inline const Byte* imageData() const
    {
        return _imageData;
    }
    
    inline Byte* imageData()
    {
        return _imageData;
    }
    
    inline const Size& size() const
    {
        return _size;
    }
    
    inline ImageComponentFormat format() const
    {
        return _format;
    }
    
    inline uint32_t channels() const
    {
        switch (_format)
        {
            case ImageComponentFormat::RGB:
            case ImageComponentFormat::BGR:
                return 3;
            case ImageComponentFormat::RGBA:
            case ImageComponentFormat::BGRA:
            case ImageComponentFormat::ABGR:
                return 4;
        }
    }
    
    inline bool isValid() const
    {
        return _imageData != nullptr;
    }
    
    void destroy();
    
protected:
    Byte* _imageData = nullptr;
    Size _size;
    ImageComponentFormat _format = ImageComponentFormat::RGB;
    
    static std::unordered_map<std::string,std::shared_ptr<Image>> g_imageCache;

private:
    enum class ImageSource {
        None = 0,
        Setter,
        LoadFromFile
    };
    
    ImageSource _imageSource = ImageSource::None;
};

}
}

#endif

