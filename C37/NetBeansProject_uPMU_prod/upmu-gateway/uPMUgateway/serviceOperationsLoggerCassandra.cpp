/*

 */


/* 
 * File:   serviceOperationsLoggerCassandra.cpp
 * Author: mcp
 * 
 * Created on April 30, 2016, 1:54 AM
 */
#include <mutex>
#include <map>
#include <boost/thread.hpp>
#include <boost/any.hpp>
#include "cassandra.h"
#include "upmuStructKeyValueListProtobuf.pb.h"
#include "serviceOperationsLoggerCassandra.h"

using namespace boost;

namespace serviceCommon {
serviceOperationsLoggerCassandra::serviceOperationsLoggerCassandra() {
}

serviceOperationsLoggerCassandra::serviceOperationsLoggerCassandra(const serviceOperationsLoggerCassandra& orig) {
}

serviceOperationsLoggerCassandra::~serviceOperationsLoggerCassandra() {
}

int serviceOperationsLoggerCassandra::cassandraArchive(const CassPrepared * prepared, CassSession *session,
       unsigned long timestampMsec, int sessionID, std::string component,
        std::string device, unsigned int domainType, unsigned int msgType,
        void * buffer, unsigned int byteLen) {       
     
    CassStatement * statement = cass_prepared_bind(prepared);
    /* Bind the values using the indices of the bind variables */
    cass_statement_bind_int64_by_name(statement, "SESSION_ID", sessionID);
    cass_statement_bind_string_by_name(statement, "COMPONENT", component.c_str());
    cass_statement_bind_int64_by_name(statement, "TIMESTAMP_MSEC", timestampMsec);
    cass_statement_bind_int64_by_name(statement, "DAY", (timestampMsec/86400));
    cass_statement_bind_string_by_name(statement, "DEVICE", device.c_str());
    cass_statement_bind_int32_by_name(statement, "DOMAIN_TYPE", domainType);
    cass_statement_bind_int32_by_name(statement, "MSG_TYPE", msgType);
    cass_statement_bind_bytes_by_name(statement, "DATA", (const cass_byte_t *)buffer, byteLen);
    
    CassFuture* cassQuery_future = cass_session_execute(session, statement);

    /* This will block until the query has finished */
    CassError rc = cass_future_error_code(cassQuery_future);
    /* Statement objects can be freed immediately after being executed */
    cass_statement_free(statement);
    if(rc != CASS_OK) {
        return FAIL;
    }
    return SUCCESS;
} 
}
