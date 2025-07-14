//
// OpenFileDialog.hpp
#pragma once

#include <bg2e/common.hpp>

#include <filesystem>
#include <map>
#include <string>

namespace bg2e {
namespace app {

class BG2E_API FileDialog {
public:
    using FileFilters = std::map<std::string, std::string>;
    
    static void test();
    
    std::filesystem::path openFile();
    
    std::filesystem::path saveFile();
    
    // TODO: Pass default selected folder
    std::filesystem::path pickFolder();

    void setFilters(const FileFilters& filters);
    
protected:
    FileFilters _filters;
};

}
}
