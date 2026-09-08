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
#include <stdexcept>

namespace bg2e::base {

std::filesystem::path PlatformTools::applicationPath()
{
    @autoreleasepool {
        NSString * bundlePath = [[NSBundle mainBundle] bundlePath];
        if (bundlePath == nil)
        {
            throw std::runtime_error("Unable to resolve the application bundle path");
        }
        auto path = std::filesystem::path([bundlePath fileSystemRepresentation]);
        if (!path.is_absolute())
        {
            throw std::runtime_error("The resolved application bundle path is not absolute");
        }
        return path.lexically_normal();
    }
}

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

std::filesystem::path PlatformTools::homePath()
{
    std::filesystem::path basePath;

    @autoreleasepool {
        NSString * homeDir = NSHomeDirectory();
        basePath = std::filesystem::path([homeDir UTF8String]);
    }

    return basePath;
}

}
