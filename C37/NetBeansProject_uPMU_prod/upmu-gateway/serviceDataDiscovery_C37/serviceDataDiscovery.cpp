/*
*** Copyright Notice ***
If you have questions about your rights to use or distribute this software, please contact Berkeley Lab's Intellectual Property Office at  IPO@lbl.gov.
NOTICE.  This Software was developed under funding from the U.S. Department of Energy and the U.S. Government consequently retains certain rights. As such, the U.S. Government has been granted for itself and others acting on its behalf a paid-up, nonexclusive, irrevocable, worldwide license in the Software to reproduce, distribute copies to the public, prepare derivative works, and perform publicly and display publicly, and to permit other to do so. 
****************************
*** License Agreement ***
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
(1) Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
(2) Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
(3) Neither the name of the University of California, Lawrence Berkeley National Laboratory, U.S. Dept. of Energy, nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
You are under no obligation whatsoever to provide any bug fixes, patches, or upgrades to the features, functionality or performance of the source code ("Enhancements") to anyone; however, if you choose to make your Enhancements available either publicly, or directly to Lawrence Berkeley National Laboratory, without imposing a separate written license agreement for such Enhancements, then you hereby grant the following license: a  non-exclusive, royalty-free perpetual license to install, use, modify, prepare derivative works, incorporate into other computer software, distribute, and sublicense such enhancements or derivative works thereof, in binary and source code form.
---------------------------------------------------------------

 */


/* 
 * File:   serviceInstanceTemplateApp2.cpp
 * Author: mcp
 * 
 * Created on January 24, 2016, 5:01 PM
 */
#include <stdint.h>
#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>
#include <atomic>
#include <set>
#include <mutex>
#include <pqxx/pqxx>
#include "cassandra.h"
#include <curl/curl.h>
#include <boost/make_shared.hpp>
#include <boost/thread/thread.hpp>
#include <boost/ref.hpp>
#include <boost/any.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/dll/alias.hpp>
#include <boost/thread/condition.hpp>
#include <boost/asio.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/async_result.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/crc.hpp>      // for boost::crc_basic, boost::crc_optimal

#include <log4cpp/Appender.hh>
#include <log4cpp/Category.hh>
#include <log4cpp/FileAppender.hh>
#include <log4cpp/OstreamAppender.hh>
#include <log4cpp/Layout.hh>
#include <log4cpp/BasicLayout.hh>
#include <log4cpp/Priority.hh>

#include "dbConnection.h"
#include "domainAndMessageTypes.h"
#include "serviceCommand.h"
#include "serviceCommandQueueTemplate.h"
#include "serviceInstanceApi.h"
#include "serviceInstance.h"
#include "serviceInstanceList.h"
#include "serviceStatus.h"
#include "serviceDataBuffer.h"
#include "serviceOperationsLoggerPostgres.h"
#include "serviceOperationsLoggerCassandra.h"
#include "upmuFtpAccess.h"
#include "upmuStructKeyValueListProtobuf.pb.h"

using namespace serviceCommon;

#include "C37_Buffer.h"
#include "C37_inputDesc.h"
#include "C37_CfgDescriptor.h"
#include "C37_MsgFormats.h"
#include "C37_Reformat.h"
#include "C37_DataChannel.h"
#include "serviceCommonStats.h"
#include "serviceSpecificStats.h"
#include "uPMUgmtime.h"

#include "serviceDataDiscovery.h"

#define IS_BIG_ENDIAN (*(uint16_t *)"\0\xff" < 0x100)

namespace serviceDataDiscovery {
using namespace pqxx;
using namespace serviceCommon;
using namespace boost;
  
    
serviceDataDiscovery::serviceDataDiscovery() {
    std::cout<< "Creating serviceDataDiscovery" << std::endl;
    cmdQueue = new SynchronisedCommandQueue<SERVICE_COMMAND *>(CMD_QUEUE_LEN);
    m_thread=NULL;
    m_mustStop=false;
    m_msg_num=0;
    serviceStatus = NOT_STARTED;
    
}

serviceDataDiscovery::~serviceDataDiscovery() {
    std::cout<< "Destroying serviceDataDiscovery" << std::endl;
    if (m_thread!=NULL) delete m_thread;
}

// Start the thread 
void serviceDataDiscovery::start() {
    // Create thread and start it with myself as argument.
    // Pass myself as reference since I don't want a copy
    m_mustStop = false;
    /* start thread.  operator() () is the function executed */
    m_thread=new boost::thread(boost::ref(*this));
}
 
// Stop the thread
void serviceDataDiscovery::stop()
{
    // Signal the thread to stop (thread-safe)
    serviceStatus = STOPPING;
    m_mustStopMutex.lock();
    m_mustStop=true;
    m_mustStopMutex.unlock();
    // notify running thread
    wake_up.notify_one();
 
    // Wait for the thread to finish.
    if (m_thread!=NULL) m_thread->join();
}

dataBufferOffer_t serviceDataDiscovery::dataBufferOffer(
    const std::shared_ptr<serviceDataBuffer>& offeredBuf) {    
    //std::cout<< "serviceDataDiscovery::dataBufferOffer" << std::endl;  
    /* this service does not accept buffers */
    return BUFFER_REFUSED;
}
 
// Thread function
void serviceDataDiscovery::operator () () {
    /* init some local flags */
    bool localMustStop = false;
    SERVICE_COMMAND * cmd;
    serviceStatus = RUNNING;   
    /* create log4cpp related appender objects for this service. */
    logger_serviceDataDiscovery.addAppender(logger_appender);
    /* log startup message */
    logger_serviceDataDiscovery.info("serviceDataDiscovery starting.");
    
    if(!initService()) {
        /* log error and exit*/
        logger_serviceDataDiscovery.error("Could not complete initialization, exiting.");
        return;
    }
    
    try {
        while(!localMustStop) {
            /* always check for possible command */
            cmd = cmdQueue->Dequeue();
            if(cmd != 0) {
                processCmd(cmd);
            }
            /* sleep for a bit. Interval changes based on operating mode. */
            std::this_thread::sleep_for(std::chrono::seconds(1));
            // Get the "must stop" state (thread-safe)
            m_mustStopMutex.lock();
            localMustStop=m_mustStop;
            m_mustStopMutex.unlock();
        }
        // exiting function kills thread
        serviceStatus = NORMAL_STOP;
        logger_serviceDataDiscovery.info("Normal (requested) service stop.");
        return;
    }
    catch (const std::exception &e) {
        serviceStatus = ERROR_STOP;
        logger_serviceDataDiscovery.error("Abnormal (error) service stop.");
        return;
    }
};

void serviceDataDiscovery::processCmd(SERVICE_COMMAND * cmd) {
    std::cout<< "         Process a command." << std::endl;
    
}

bool serviceDataDiscovery::initService() {
    /* init service data structures */
    createServiceCommonStats(this);
    createServiceSpecificStats(this);
    
    /* get a db connection for local database */
    dbConn = dbConnection::getInstance();
    if(dbConn->conn == NULL) {
        /* log an error and exit. */
        return false;
    }
    
    /* prepare any postgres statements used regularly */
    dbConn->acquireDBaccess();
    (dbConn->conn)->prepare("add_dir_record", 
            "INSERT INTO upmu_data_directory (year, month, day, hour, "
                "dir_name, state, discovery_time, release_time) VALUES "
                "($1, $2, $3, $4, $5, $6, "
                "NOW(), NULL)");
    (dbConn->conn)->prepare("get_dir_record_key", 
            "SELECT currval('upmu_data_directory_id_seq')");
    (dbConn->conn)->prepare("release_dir_record", 
            "UPDATE upmu_data_directory SET (state, release_time) = ('processed', NOW()) WHERE id = $1");
    (dbConn->conn)->prepare("abandon_dir_record", 
            "UPDATE upmu_data_directory SET (state, release_time) = ('abandoned', NOW()) WHERE id = $1");
    (dbConn->conn)->prepare("update_dir_record_state", 
            "UPDATE upmu_data_directory SET state = $1 WHERE id = $2");
    (dbConn->conn)->prepare("add_data_record",
            "INSERT INTO upmu_data_file (file_name, directory, state, "
            "access_start_time, release_time) VALUES "
            "($1, $2, $3, NOW(), NULL)");
    (dbConn->conn)->prepare("release_data_record",
            "UPDATE upmu_data_file SET (state, release_time) = ('released', NOW()) WHERE id = $1");
    (dbConn->conn)->prepare("abandon_data_record", 
            "UPDATE upmu_data_file SET (state, release_time) = ('abandoned', NOW()) WHERE id = $1");
    (dbConn->conn)->prepare("get_data_record_key", 
            "SELECT currval('upmu_data_file_id_seq')");
    (dbConn->conn)->prepare("search_data_record_by_dir_index",
            "SELECT * FROM upmu_data_file WHERE directory = $1");
    dbConn->releaseDBaccess();
    logger_serviceDataDiscovery.info("Postgres statements prepared.");
         
    /* declare uPMUgateway started at this point. Note: startup message always 
     contains sessionID and major/minor version info.  Also, these params are ad hoc
     in the gateway; but are part of standard plugin statistices package in plugins. */
    postgresOperationsLogger->createStringOperationsRecord(dbConn, serviceState, upmuSerialNumber,
        sessionID, serviceName, "Service_Startup_Msg", "Initialized, looking for current directory on uPMU.");
    postgresOperationsLogger->createStringOperationsRecord(dbConn, serviceState, upmuSerialNumber,
        sessionID, serviceName, 
        (*serviceSpecStatsLabels)[INDX_STR_DATA_DISCOVERY_MAJ_VER]->c_str(), 
        any_cast<std::string *>((*serviceSpecStatsValues)[INDX_STR_DATA_DISCOVERY_MAJ_VER])->c_str());
    postgresOperationsLogger->createStringOperationsRecord(dbConn, serviceState, upmuSerialNumber,
        sessionID, serviceName, 
        (*serviceSpecStatsLabels)[INDX_STR_DATA_DISCOVERY_MIN_VER]->c_str(), 
        any_cast<std::string *>((*serviceSpecStatsValues)[INDX_STR_DATA_DISCOVERY_MIN_VER])->c_str());
    
    if(cassArchivingEnabled && cassConnected) {
        /* make a KeyValueList to log similar message to cassandra, if available.*/
        upmuStructKeyValueList * kvList = new upmuStructKeyValueList();
        createServiceStartupKeyValueList(kvList);
        uint32_t byteLen = kvList->ByteSize();
        void * pbBuffer = std::malloc(byteLen);
        if(pbBuffer != nullptr) {
            kvList->SerializeToArray(pbBuffer, byteLen);
            cassandraOperationsLogger->cassandraArchive(cassPreparedOperations, cassSession,
                (uint64_t)kvList->timestamp(), sessionID, serviceName,
                upmuSerialNumber, DOMAIN_OPERATIONS, MSG_KEY_VALUE_LIST, pbBuffer, byteLen);
            try{
                std::free(pbBuffer);
            }
            catch(...){
                std::cout<<"fail free buffer discovery";
            }
        } 
        else {
            logger_serviceDataDiscovery.error("Cannot form serialized protocol buffer, "
                "insufficient memory."); 
        }
    }
    /* create endpoint that points to uPMU address */  
    boost::thread readData(&serviceDataDiscovery::processUPMUdata, this, ftpIpAddress);
    return true;
}

void serviceDataDiscovery::processUPMUdata(std::string ipAddress){
    /* note: this runs as a separate thread.  boost::asio needs that.
     Operations are started by c37.connect().  Incoming data is handled
     asynchronously.  To check for auto reconnect, look in C37_DataChannel. */
    /* create asio infrastructure */
    boost::asio::io_service io;
    boost::asio::io_service::work work(io);
    boost::asio::ip::tcp::endpoint endpoint(
        boost::asio::ip::address::from_string(ipAddress), 4713);
    C37_DataChannel c37in(io, endpoint);
    c37in.connect();
    
    try{
        /* this allows asio to post asychrounous callbacks. */
        io.run();
    }
    catch (std::exception ex) {
        int k = 0;
    }
    
    
}

void serviceDataDiscovery::createServiceStartupKeyValueList(upmuStructKeyValueList * kvList) {
    kvList->set_name("Service_Startup_Info");
    uint32_t startTime = (uint32_t)time(NULL);
    kvList->set_timestamp(startTime);
    KeyValueList * list = kvList->add_list();
    list->set_category("ServiceStatus");

    /* add a few key values */
    serviceSpecStatsMutex.lock();
    KeyValue * pair = list->add_element();
    pair->set_key((*serviceSpecStatsLabels)[INDX_STR_DATA_DISCOVERY_MAJ_VER]->c_str()); 
    pair->set_stringval(any_cast<std::string *>
                ((*serviceSpecStatsValues)[INDX_STR_DATA_DISCOVERY_MAJ_VER])->c_str());

    pair = list->add_element();
    pair->set_key((*serviceSpecStatsLabels)[INDX_STR_DATA_DISCOVERY_MIN_VER]->c_str()); 
    pair->set_stringval(any_cast<std::string *>
                ((*serviceSpecStatsValues)[INDX_STR_DATA_DISCOVERY_MIN_VER])->c_str());
    serviceSpecStatsMutex.unlock();
}

void serviceDataDiscovery::swapBytesUint(unsigned int * val) {
    unsigned int temp = *val;
    *val =  ((((temp) & 0xff000000) >> 24) |
                (((temp) & 0x00ff0000) >>  8) |
                (((temp) & 0x0000ff00) <<  8) |
                (((temp) & 0x000000ff) << 24));
}


int serviceDataDiscovery::cassandraCommonArchive(const CassPrepared * prepared,
        uint32_t statsIndex,
        uint64_t timestampMsec, uint64_t day,
        std::string device, uint32_t domainType, uint32_t msgType,
        void * buffer, uint32_t byteLen) {
     
    CassStatement * statement = cass_prepared_bind(prepared);
    /* Bind the values using the indices of the bind variables */
    cass_statement_bind_int64_by_name(statement, "TIMESTAMP_MSEC", timestampMsec);
    cass_statement_bind_int64_by_name(statement, "DAY", day);
    cass_statement_bind_string_by_name(statement, "DEVICE", device.c_str());
    cass_statement_bind_int32_by_name(statement, "DOMAIN_TYPE", domainType);
    cass_statement_bind_int32_by_name(statement, "MSG_TYPE", msgType);
    cass_statement_bind_bytes_by_name(statement, "DATA", (const cass_byte_t *)buffer, byteLen);
    
    CassFuture* cassQuery_future = cass_session_execute(cassSession, statement);

    /* This will block until the query has finished */
    CassError rc = cass_future_error_code(cassQuery_future);
    if(rc != CASS_OK) {
        /* count errors for each cassandra table */
        serviceSpecStatsMutex.lock();
        unsigned int * cnt = any_cast<unsigned int *>
            ((*serviceSpecStatsValues)[statsIndex]);
        *cnt++;
        serviceSpecStatsMutex.unlock();
    }
    /* Statement objects can be freed immediately after being executed */
    cass_statement_free(statement);
    cass_future_free(cassQuery_future);
}

serviceDataDiscovery plugin;
BOOST_DLL_AUTO_ALIAS(plugin)

}
