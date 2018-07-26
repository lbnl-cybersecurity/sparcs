/*
 */

/* 
 * File:   serviceInstanceApi.h
 * Author: mcp
 *
 * Created on January 19, 2016, 12:11 AM
 */

#ifndef SERVICEINSTANCEAPI_H
#define SERVICEINSTANCEAPI_H

#include <string>
#include "serviceStatus.h"
#include "serviceCommand.h"
#include "serviceDataBuffer.h"

namespace serviceCommon {
    
class serviceInstanceApi {
public:
    SynchronisedCommandQueue<SERVICE_COMMAND *> * cmdQueue;
    boost::mutex serviceCmnStatsMutex;
    std::map<unsigned int, std::string *> * serviceCmnStatsLabels;
    std::map<unsigned int, boost::any> * serviceCmnStatsValues;
    boost::mutex serviceSpecStatsMutex;
    std::map<unsigned int, std::string *> * serviceSpecStatsLabels;
    std::map<unsigned int, boost::any> * serviceSpecStatsValues;

    boost::condition_variable wake_up;
    serviceState_t serviceStatus;
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual dataBufferOffer_t dataBufferOffer(const std::shared_ptr<serviceDataBuffer>&) = 0;
    virtual void operator () () = 0;
    virtual void setServiceName(std::string) = 0;
    virtual std::string getServiceName() = 0;
    virtual ~serviceInstanceApi() {};
};
}

#endif /* SERVICEINSTANCEAPI_H */

