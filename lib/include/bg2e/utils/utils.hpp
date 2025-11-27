//
//  utils.hpp
#pragma once

#include <bg2e/common.hpp>

#include <string>

namespace bg2e {
namespace utils {

/**
    uniqueId: generate an unique identifier in the same format as the RFC 4122, but this
    is NOT an RFC 4122 uuid. Is a quick method to generate identifiers that are unique in
    the local device. These identifiers have been created to share the same format as a
    UUID in case in the future you want to replace the algorithm to generate real UUIDs.
 */
extern BG2E_API std::string uniqueId();

}
}
