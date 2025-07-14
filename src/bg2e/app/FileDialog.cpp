//
//  OpenFileDialog.cpp
#include <bg2e/app/FileDialog.hpp>

#include <iostream>
#include <nfd.hpp>

namespace bg2e::app {

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
    
void FileDialog::setFilters(const FileFilters& filters)
{
    _filters = filters;
}

}
