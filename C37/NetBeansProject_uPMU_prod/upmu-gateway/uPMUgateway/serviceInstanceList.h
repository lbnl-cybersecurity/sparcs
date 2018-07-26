/*
 */

/* 
 * File:   serviceInstanceList.h
 * Author: mcp
 *
 * Created on January 25, 2016, 7:46 AM
 */

#ifndef SERVICEINSTANCELIST_H
#define SERVICEINSTANCELIST_H

namespace serviceCommon{
    
class serviceInstanceList {
public:
    static serviceInstanceList* getInstance();
    void createServiceInstance( serviceInstance * si);
    std::map<std::string, serviceCommon::serviceInstance *> services;
    ~serviceInstanceList();
private:
    static bool instanceFlag;
    static serviceInstanceList *single;
    serviceInstanceList();
    std::mutex m_mutex; // The mutex to synchronise on
};

}
#endif /* SERVICEINSTANCELIST_H */

