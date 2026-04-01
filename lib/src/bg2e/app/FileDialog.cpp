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

#include <bg2e/app/FileDialog.hpp>
#include <bg2e/base/PlatformTools.hpp>

#include <iostream>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdocumentation"
#endif

#include <nfd.hpp>

#ifdef __clang__
#pragma clang diagnostic pop
#endif

namespace bg2e::app {

FileDialog::FileFilters FileDialog::imageFilters {
    { "Images", "jpg,jpeg,png,bmp,webp" }
};

void FileDialog::test()
{
    NFD::Guard nfdGuard;
    
    NFD::UniquePath outPath;
    
    nfdfilteritem_t filterItem[1] = {{ "bg2e model files", "bg2,vwglb" }};
    
    nfdresult_t result = NFD::OpenDialog(outPath, filterItem, 1);
    if (result == NFD_OKAY)
    {
        std::cout << "Success!" << std::endl << outPath.get() << std::endl;
    }
    else if (result == NFD_CANCEL)
    {
        std::cout << "User pressed cancel." << std::endl;
    }
    else
    {
        std::cout << "Error: " << NFD::GetError() << std::endl;
    }
}

std::filesystem::path FileDialog::openFile()
{
    if (_filters.size() == 0)
    {
        throw std::runtime_error("FileDialog::openFile(): No filters specified");
    }
    
    NFD::Guard();
    
    NFD::UniquePath outPath;
    
    nfdfilteritem_t * filters = new nfdfilteritem_t[_filters.size()];
    size_t i = 0;
    for (auto & f : _filters)
    {
        filters[i].name = f.first.c_str();
        filters[i].spec = f.second.c_str();
        
        ++i;
    }
    std::filesystem::path result;
    nfdresult_t nfdResult= NFD::OpenDialog(outPath, filters, static_cast<nfdfiltersize_t>(_filters.size()));
    
    if (nfdResult == NFD_OKAY)
    {
        result = outPath.get();
    }
    else if (nfdResult != NFD_CANCEL)
    {
        throw std::runtime_error("Error opening file dialog:" + std::string(NFD::GetError()));
    }
    
    delete [] filters;
    return result;
}
    
std::filesystem::path FileDialog::saveFile()
{
    if (_filters.size() == 0)
    {
        throw std::runtime_error("FileDialog::openFile(): No filters specified");
    }
    
    NFD::Guard();
    
    NFD::UniquePath outPath;
    
    nfdfilteritem_t * filters = new nfdfilteritem_t[_filters.size()];
    size_t i = 0;
    for (auto & f : _filters)
    {
        filters[i].name = f.first.c_str();
        filters[i].spec = f.second.c_str();
        
        ++i;
    }
    std::filesystem::path result;
    nfdresult_t nfdResult= NFD::SaveDialog(outPath, filters, static_cast<nfdfiltersize_t>(_filters.size()));
    
    if (nfdResult == NFD_OKAY)
    {
        result = outPath.get();
    }
    else if (nfdResult != NFD_CANCEL)
    {
        throw std::runtime_error("Error opening file dialog:" + std::string(NFD::GetError()));
    }
    
    delete [] filters;
    return result;
}

std::filesystem::path FileDialog::pickFolder()
{
    NFD::Guard nfdGuard;
    
    NFD::UniquePath outPath;
    std::filesystem::path result;
    
    nfdresult_t nfdResult = NFD::PickFolder(outPath);
    if (nfdResult == NFD_OKAY)
    {
        result = outPath.get();
    }
    else if (nfdResult != NFD_CANCEL)
    {
        throw std::runtime_error("Error picking directory: " + std::string(NFD::GetError()));
    }
    
    return result;
}
    
void FileDialog::setFilters(const FileFilters& filters)
{
    _filters = filters;
}

std::filesystem::path FileDialog::getOpenFilePath(const FileFilters& filters)
{
    FileDialog fd;
    fd.setFilters(filters);
    return fd.openFile();
}

std::filesystem::path FileDialog::getSaveFilePath(const FileFilters& filters)
{
    FileDialog fd;
    fd.setFilters(filters);
    return fd.saveFile();
}

std::filesystem::path FileDialog::getPickFolderPath()
{
    FileDialog fd;
    return fd.pickFolder();
}

}
