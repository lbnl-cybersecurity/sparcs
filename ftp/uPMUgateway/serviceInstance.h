/*
 */

/* 
 * File:   serviceInstance.h
 * Author: mcp
 *
 * Created on January 18, 2016, 7:54 PM
 */

#ifndef SERVICEINSTANCE_H
#define SERVICEINSTANCE_H

namespace serviceCommon {
    

class serviceInstance {
public:
    serviceInstance();
    serviceInstance(std::string serviceName, std::string serviceImplementation,
        std::string serviceLibPath, bool serviceAcceptsData,
        int serviceMask, int serviceQueueDepth);
    serviceInstance(const serviceInstance& orig);
    virtual ~serviceInstance();
    std::string serviceName;
    std::string serviceImplementation;
    std::string serviceLibPath;
    bool serviceAcceptsData;
    int serviceMask;
    int serviceQueueDepth;
    boost::shared_ptr<serviceInstanceApi>serviceThread;
    
private:

};

}

#endif /* SERVICEINSTANCE_H */

