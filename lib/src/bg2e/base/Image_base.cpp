/*
 *    business grade graphic engine (bg2 engine)
 *    Copyright (C) 2026  Fernando Serrano Carpena
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <bg2e/base/Image.hpp>

namespace bg2e::base {

Image::Image()
{

}

Image::Image(unsigned char* data, uint32_t width, uint32_t height, uint32_t channels)
    :_data(data)
    ,_dataf(nullptr)
    ,_width(width)
    ,_height(height)
    ,_channels(channels)
{

}

Image::Image(float* data, uint32_t width, uint32_t height, uint32_t channels)
    :_data(nullptr)
    ,_dataf(data)
    ,_width(width)
    ,_height(height)
    ,_channels(channels)
{

}

Image::~Image()
{
    cleanup();
}
    
void Image::setData(unsigned char* data, uint32_t width, uint32_t height, uint32_t channels)
{
    cleanup();
    
    _data = data;
    _dataf = nullptr;
    _width = width;
    _height = height;
    _channels = channels;
}

void Image::setData(float* data, uint32_t width, uint32_t height, uint32_t channels)
{
    cleanup();
    
    _data = nullptr;
    _dataf = data;
    _width = width;
    _height = height;
    _channels = channels;
}

void Image::cleanup()
{
    if (_data)
    {
        delete [] _data;
        _data = nullptr;
    }
    
    if (_dataf)
    {
        delete [] _dataf;
        _dataf = nullptr;
    }
    _width = 0;
    _height = 0;
    _channels = 0;
}

}
