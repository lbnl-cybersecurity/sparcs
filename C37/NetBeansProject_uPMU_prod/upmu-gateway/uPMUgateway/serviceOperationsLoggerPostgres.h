/*
 */

/* 
 * File:   serviceOperationsLoggerPostgres.h
 * Author: mcp
 *
 * Created on April 29, 2016, 9:17 PM
 */

#ifndef SERVICEOPERATIONSLOGGERPOSTGRES_H
#define SERVICEOPERATIONSLOGGERPOSTGRES_H

using namespace pqxx;

namespace serviceCommon {
    
enum DataType {data_string, data_unsignedInt, data_int, data_float};

class serviceOperationsLoggerPostgres {
public:
    serviceOperationsLoggerPostgres(dbConnection * dbConn);
    serviceOperationsLoggerPostgres(const serviceOperationsLoggerPostgres& orig);
    virtual ~serviceOperationsLoggerPostgres();
    int createStringOperationsRecord(dbConnection * dbConn, std::string state, std::string device,
        int sessionID, std::string component, std::string key, std::string value);
    int createFloatOperationsRecord(dbConnection * dbConn, std::string state, std::string device,
        int sessionID, std::string component, std::string key, float value);
    int createIntOperationsRecord(dbConnection * dbConn, std::string state, std::string device,
        int sessionID, std::string component, std::string key, int value);
private:

};
}


#endif /* SERVICEOPERATIONSLOGGERPOSTGRES_H */

