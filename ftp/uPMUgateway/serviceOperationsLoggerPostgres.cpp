/*
 * Stream-Processing Architecture for Real-time Cyber-physical Security (SPARCS)
 */


/* 
 * File:   serviceOperationsLoggerPostgres.cpp
 * Author: mcp
 * 
 * Created on April 29, 2016, 9:17 PM
 */
#include <cstdlib>
#include <iostream>
#include <pqxx/pqxx>

#include "dbConnection.h"
#include "serviceOperationsLoggerPostgres.h"

using namespace std;
using namespace pqxx;

#define SUCCESS 0
#define FAIL -1

namespace serviceCommon {
    
serviceOperationsLoggerPostgres::serviceOperationsLoggerPostgres(dbConnection * dbConn) {
    /* prepare for possible failure */
    bool prepareSuccessful = true;
    /* prepare any statements used regularly */
    dbConn->acquireDBaccess();
    try {
    (dbConn->conn)->prepare("add_operations_record", 
            "INSERT INTO operations (state, device, session_id, "
                "component, key, value_type, value_string, value_float, "
                "value_int, last_modified) VALUES "
                "($1, $2, $3, $4, $5, $6, $7, $8, $9, NOW())");
    }
    catch (std::exception e) {
        prepareSuccessful = false;
    }
    dbConn->releaseDBaccess();
    if(!prepareSuccessful) {
        throw std::domain_error("Cannot prepare postgres statement.");
    }  
}

serviceOperationsLoggerPostgres::serviceOperationsLoggerPostgres(const serviceOperationsLoggerPostgres& orig) {
}

serviceOperationsLoggerPostgres::~serviceOperationsLoggerPostgres() {
}

int serviceOperationsLoggerPostgres::createStringOperationsRecord(
    dbConnection * dbConn, std::string state, std::string device,
        int sessionID, std::string component, std::string key, std::string value) {
    dbConn->acquireDBaccess();
    try {
        work w(*(dbConn->conn));
        w.prepared("add_operations_record")(state.c_str())(device.c_str())\
            (sessionID)(component.c_str())(key.c_str())("string")(value.c_str())\
            ()().exec();
        w.commit();
        
        dbConn->releaseDBaccess();
        return SUCCESS;
    }
    catch (std::exception& e) {
        std::cout  << e.what() << std::endl;
        dbConn->releaseDBaccess();
    }
}

int serviceOperationsLoggerPostgres::createFloatOperationsRecord(
    dbConnection * dbConn, std::string state, std::string device,
        int sessionID, std::string component, std::string key, float value) {
    dbConn->acquireDBaccess();
    try {
        work w(*(dbConn->conn));
        w.prepared("add_operations_record")(state.c_str())(device.c_str())\
            (sessionID)(component.c_str())(key.c_str())("string")()\
            (value)().exec();
        w.commit();
        
        dbConn->releaseDBaccess();
        return SUCCESS;
    }
    catch (std::exception& e) {
        std::cout  << e.what() << std::endl;
        dbConn->releaseDBaccess();
    }
}

int serviceOperationsLoggerPostgres::createIntOperationsRecord(
    dbConnection * dbConn, std::string state, std::string device,
        int sessionID, std::string component, std::string key, int value) {
    dbConn->acquireDBaccess();
    try {
        work w(*(dbConn->conn));
        w.prepared("add_operations_record")(state.c_str())(device.c_str())\
            (sessionID)(component.c_str())(key.c_str())("string")()\
            ()(value).exec();
        w.commit();
        
        dbConn->releaseDBaccess();
        return SUCCESS;
    }
    catch (std::exception& e) {
        std::cout  << e.what() << std::endl;
        dbConn->releaseDBaccess();
    }
}
}
