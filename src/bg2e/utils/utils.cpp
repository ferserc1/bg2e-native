//
//  utils.cpp

#include <bg2e/utils/utils.hpp>

#include <random>
#include <chrono>
#include <functional>

namespace bg2e::utils
{

std::string uniqueId()
{
    auto now = std::chrono::system_clock::now();
    auto timestamp = now.time_since_epoch().count();
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint64_t> dis(0, UINT64_MAX);
    uint64_t random = dis(gen);
    
    uint64_t mixed = timestamp ^ random;
    
    std::hash<uint64_t> hash;
    uint64_t uniqueId = hash(mixed);
    
    std::stringstream ss;
    ss << std::hex << uniqueId;
    std::string uniqueIdString = ss.str();
    
    uniqueIdString.insert(8, "-");
    uniqueIdString.insert(13, "-");
    uniqueIdString.insert(18, "-");
    uniqueIdString.insert(23, "-");
    
    return uniqueIdString;
}

}
