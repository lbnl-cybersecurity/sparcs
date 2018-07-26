/*
 */

/* 
 * File:   dbConnection.h
 * Author: mcp
 *
 * Created on January 17, 2016, 7:38 PM
 */

#ifndef DBCONNECTION_H
#define DBCONNECTION_H

    
#include <pqxx/pqxx>
#include <mutex>

using namespace pqxx;
namespace serviceCommon {

class dbConnection
{
private:
    static bool instanceFlag;
    static dbConnection *single;
    std::mutex localDBmutex;
    dbConnection();
public:
    static dbConnection* getInstance();
    pqxx::connection * conn;
    bool attemptAcquireDBaccess();
    void acquireDBaccess();
    void releaseDBaccess();
    ~dbConnection(); 
};    

}


#endif /* DBCONNECTION_H */

