
#include <bg2db/image_write.hpp>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <bg2db/stb_image_write.h>

#include <ios>
#include <exception>

namespace bg2db {

    void writeImage(const bg2base::path & dest, const bg2base::image & img) {
        if (!img.valid()) {
            throw std::invalid_argument("bg2db::writeImage(): invalid image.");
        }
        
        std::string ext = dest.lowerCaseExtension();
        const char * dst = dest.toString().c_str();
        const void * data = img.data();
        int w = img.size().x();
        int h = img.size().y();
        int comp = img.bytesPerPixel();
        int retValue = 0;
        if (ext == "png") {
            retValue = stbi_write_png(dst, w, h, comp, data, comp);
        }
        else if (ext == "bmp") {
            retValue = stbi_write_bmp(dst, w, h, comp, data);
        }
        else if (ext == "tga") {
            retValue = stbi_write_tga(dst, w, h, comp, data);
        }
        else if (ext == "jpg" || ext == "jpeg") {
            retValue = stbi_write_jpg(dst, w, h, comp, data, 100);
        }
        else {
            throw std::invalid_argument("bg2db::writeImage(): incompatible image format '" + ext + "'");
        }
        
        if (retValue == 0) {
            throw std::ios_base::failure("bg2db::writeImage(): error saving image '" + dest.toString() + "'");
        }
    }
    
    void writeImage(const std::string & dest, const bg2base::image & img) {
        writeImage(bg2base::path(dest), img);
    }
    
    void writeImage(const char * dest, const bg2base::image & img) {
        writeImage(bg2base::path(dest), img);
    }
    
    void writeImage(const bg2base::path & dest, const bg2base::image * img) {
        if (img != nullptr && !img->valid()) {
            throw std::invalid_argument("bg2db::writeImage(): invalid image.");
        }
        
        std::string ext = dest.lowerCaseExtension();
        const char * dst = dest.toString().c_str();
        const void * data = img->data();
        int w = img->size().x();
        int h = img->size().y();
        int comp = img->bytesPerPixel();
        int retValue = 0;
        if (ext == "png") {
            retValue = stbi_write_png(dst, w, h, comp, data, comp);
        }
        else if (ext == "bmp") {
            retValue = stbi_write_bmp(dst, w, h, comp, data);
        }
        else if (ext == "tga") {
            retValue = stbi_write_tga(dst, w, h, comp, data);
        }
        else if (ext == "jpg" || ext == "jpeg") {
            retValue = stbi_write_jpg(dst, w, h, comp, data, 100);
        }
        else {
            throw std::invalid_argument("bg2db::writeImage(): incompatible image format '" + ext + "'");
        }
        
        if (retValue == 0) {
            throw std::ios_base::failure("bg2db::writeImage(): error saving image '" + dest.toString() + "'");
        }
    }
    
    void writeImage(const std::string & dest, const bg2base::image * img) {
        writeImage(bg2base::path(dest), img);
    }
    
    void writeImage(const char * dest, const bg2base::image * img) {
        writeImage(bg2base::path(dest), img);
    }
}
