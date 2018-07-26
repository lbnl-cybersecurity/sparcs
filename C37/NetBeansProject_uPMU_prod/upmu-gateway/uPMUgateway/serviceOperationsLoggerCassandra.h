/*
 */

/* 
 * File:   serviceOperationsLoggerCassandra.h
 * Author: mcp
 *
 * Created on April 30, 2016, 1:54 AM
 */

#ifndef SERVICEOPERATIONSLOGGERCASSANDRA_H
#define SERVICEOPERATIONSLOGGERCASSANDRA_H

#define FAIL -1
#define SUCCESS 0

namespace serviceCommon {
 
class serviceOperationsLoggerCassandra {
public:
    serviceOperationsLoggerCassandra();
    serviceOperationsLoggerCassandra(const serviceOperationsLoggerCassandra& orig);
    virtual ~serviceOperationsLoggerCassandra();
    int cassandraArchive(const CassPrepared * prepared, CassSession *session,
        unsigned long timestampMsec, int sessionID, std::string component,
        std::string device, unsigned int domainType, unsigned int msgType,
        void * buffer, unsigned int byteLen);
private:

};
}

#endif /* SERVICEOPERATIONSLOGGERCASSANDRA_H */

