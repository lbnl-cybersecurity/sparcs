/*

 */


/* 
 * File:   serviceInstance.cpp
 * Author: mcp
 * 
 * Created on January 18, 2016, 7:54 PM
 */
#include <string>
#include <boost/dll/import.hpp>
#include <boost/thread/condition.hpp>
#include <boost/any.hpp>
#include "serviceCommandQueueTemplate.h"
#include "serviceInstanceApi.h"
#include "serviceInstance.h"

namespace serviceCommon {
using namespace std;
using namespace serviceCommon;
using namespace boost;
using namespace boost::dll;
    

serviceInstance::serviceInstance() {
}

serviceInstance::serviceInstance(std::string serviceName, std::string serviceImplementation,
        std::string serviceLibPath, bool serviceAcceptsData,
        int serviceMask, int serviceQueueDepth) {
    this->serviceName = serviceName;
    this->serviceImplementation = serviceImplementation;
    this->serviceLibPath = serviceLibPath;
    this->serviceAcceptsData = serviceAcceptsData;
    this->serviceQueueDepth = serviceQueueDepth;
    this->serviceMask = serviceMask;
}

serviceInstance::serviceInstance(const serviceInstance& orig) {
}

serviceInstance::~serviceInstance() {
}

}
