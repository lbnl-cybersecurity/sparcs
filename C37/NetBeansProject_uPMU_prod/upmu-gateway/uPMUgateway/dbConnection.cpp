/*

 */


/* 
 * File:   dbConnection.cpp
 * Author: mcp
 * 
 * Created on January 17, 2016, 7:38 PM
 */
    
#include <cstdlib>
#include <iostream>
#include <pqxx/pqxx>
#include "dbConnection.h"
       
using namespace std;
using namespace pqxx;


namespace serviceCommon {

bool dbConnection::instanceFlag = false;
dbConnection* dbConnection::single = NULL;

dbConnection::dbConnection() {
    try {
        
        conn = new connection("dbname=upmugateway user=upmugateway \
            password=!ForuPMUgateway hostaddr=127.0.0.1 port=5432");
        if(!conn->is_open()) {
            /* log db error */
            conn = NULL;
        }
        
    } catch(const std::exception &e) {
        /* log db error */
        std::cout << "something went wrong with postgres" << std::endl;
        std::cout << e.what();
        
        conn = NULL;
    }
}

dbConnection::~dbConnection() {
    if(conn != NULL) {
        conn->disconnect();
    }
    instanceFlag = false;
}

dbConnection* dbConnection::getInstance() {
    if(! instanceFlag)
    {
        single = new dbConnection();
        instanceFlag = true;
        return single;
    }
    else
    {
        return single;
    }
}

bool dbConnection::attemptAcquireDBaccess() {
    return localDBmutex.try_lock();
}

void dbConnection::acquireDBaccess() {
    localDBmutex.lock();
}

void dbConnection::releaseDBaccess() {
    localDBmutex.unlock();
}
}

