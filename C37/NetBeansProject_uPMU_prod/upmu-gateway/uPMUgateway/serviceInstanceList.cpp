/*

 */

/* 
 * File:   serviceInstanceList.cpp
 * Author: mcp
 * 
 * Created on January 25, 2016, 7:46 AM
 */

#include <boost/thread.hpp>
#include <boost/dll/import.hpp>
#include <boost/thread/condition.hpp>
#include <boost/any.hpp>
#include <dlfcn.h>
#include <map>
#include <mutex>
#include <iostream>
#include "serviceCommandQueueTemplate.h"
#include "serviceInstanceApi.h"
#include "serviceInstance.h"
#include "serviceInstanceList.h"

namespace serviceCommon {
    
using namespace serviceCommon;

using namespace boost;

bool serviceInstanceList::instanceFlag = false;
serviceInstanceList* serviceInstanceList::single = NULL;
    
serviceInstanceList::serviceInstanceList() {
}

serviceInstanceList::~serviceInstanceList() {
}

serviceInstanceList* serviceInstanceList::getInstance()
{
    if(! instanceFlag)
    {
        single = new serviceInstanceList();
        instanceFlag = true;
        return single;
    }
    else
    {
        return single;
    }
}

void serviceInstanceList::createServiceInstance(
    serviceInstance * si) {
    boost::filesystem::path lib_path(si->serviceLibPath);
    
    //try {
        si->serviceThread = boost::dll::import_alias<serviceInstanceApi>(
            lib_path / si->serviceImplementation,
            "plugin",
            boost::dll::load_mode::default_mode);
    //}
    //catch (const std::exception &e) {
       /* log error */
        //int i = 0;
   // }
  
    services.insert(std::make_pair(si->serviceName, si));
}

}

