//
//  bg2e.cpp
//  bg2e
//
//  Created by Fernando Serrano Carpena on 17/2/23.
//

#include <iostream>
#include "bg2e.hpp"
#include "bg2ePriv.hpp"

void bg2e::HelloWorld(const char * s)
{
    bg2ePriv *theObj = new bg2ePriv;
    theObj->HelloWorldPriv(s);
    delete theObj;
};

void bg2ePriv::HelloWorldPriv(const char * s) 
{
    std::cout << s << std::endl;
};

