//
//  PlatformTools.mm
//  bg2e
//
//  Created by Fernando Serrano Carpena on 6/11/25.
//

#include <bg2e/base/PlatformTools.hpp>
#include <bg2e/app/MainLoop.hpp>

#import <Foundation/Foundation.h>

#include <iostream>

namespace bg2e::base {

std::filesystem::path PlatformTools::settingsPath()
{
    std::filesystem::path basePath;
    auto appId = app::MainLoop::current()->appId();
    
    @autoreleasepool {
        NSArray  * paths = NSSearchPathForDirectoriesInDomains(
            NSApplicationSupportDirectory,
            NSUserDomainMask,
            YES
        );
        
        NSString * base = [paths firstObject];
        NSString * full = [base stringByAppendingPathComponent:[NSString stringWithUTF8String:appId.c_str()]];
        basePath = std::filesystem::path([full UTF8String]);
    }
    
    try {
        std::filesystem::create_directories(basePath);
    }
    catch (std::runtime_error err)
    {
        std::cerr << "Unable to create settings directory at path \"" << basePath << "\"" << std::endl;
    }
    return basePath;
}

}
