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
#include <boost/thread.hpp>
#include <boost/ref.hpp>
#include <boost/any.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/dll/alias.hpp>
#include <boost/thread/condition.hpp>
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
            processUPMUdata();
            /* sleep for a bit. Interval changes based on operating mode. */
            if(scanMode == normal) {
                std::this_thread::sleep_for(std::chrono::seconds(SCAN_MODE_NORMAL_DELAY_SEC));
            }
            else if(scanMode == catch_up) {
                std::this_thread::sleep_for(std::chrono::seconds(SCAN_MODE_CATCH_UP_DELAY_SEC));
            }
            else {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
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
     
    /* init file length tracker and initial serviceState. */
    mostRecentDataFileLength = 0;
    scanMode = catch_up;
    serviceState = STATE_CATCH_UP;
    
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
                
            }
        } 
        else {
            logger_serviceDataDiscovery.error("Cannot form serialized protocol buffer, "
                "insufficient memory."); 
        }
    }
    
    
    /* create several structures used throughout thread */
    dataDir = new DATA_DIR();
    
    int sts =getCurrentDirectory();
    if(sts != DATA_DIR_OK) {
        return false;
    }
    sts = getCurrentDataFileSet();
    if(sts != DATA_DIR_OK) {
        return false;
    }
    return true;
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

int serviceDataDiscovery::getCurrentDirectory() {
    try {
        /* get list of current data directory - if we have one */
        bool startingDirInDB = false;
        
        dbConn->acquireDBaccess();
        nontransaction n(*(dbConn->conn));
        result r = n.exec("SELECT * FROM upmu_data_directory WHERE state = 'current'");
        if(r.size() == 1) {
            dataDir->year = r[0]["\"year\""].as<int>();
            dataDir->month = r[0]["\"month\""].as<int>();
            if((dataDir->month < 1) || (dataDir->month >12)) {
                /* month not in correct range */
                n.commit();
                dbConn->releaseDBaccess();
                return DATA_DIR_ERROR;
            }
            dataDir->day = r[0]["\"day\""].as<int>();
            if(dataDir->day <= 0) {
                /* days start at 1, not in correct range */
                n.commit();
                dbConn->releaseDBaccess();
                return DATA_DIR_ERROR;
            }
            dataDir->hour = r[0]["\"hour\""].as<int>();
            dataDir->dbID = r[0]["\"id\""].as<int>();
            dataDir->dirName = upmuFtpAccess::createDirName_YrMonDayHr(dataDir);
            dataDir->state = current;
            startingDirInDB = true;
        } 
        else {
            result r = n.exec("SELECT * FROM upmu_data_directory WHERE state = 'processed'"
                    " ORDER BY id DESC LIMIT 1");
            if(r.size() == 1) {
                dataDir->year = r[0]["\"year\""].as<int>();
                dataDir->month = r[0]["\"month\""].as<int>();
                if((dataDir->month < 1) || (dataDir->month >12)) {
                    /* month not in correct range */
                    n.commit();
                    dbConn->releaseDBaccess();  
                   return DATA_DIR_ERROR;
                }
                dataDir->day = r[0]["\"day\""].as<int>();
                if(dataDir->day <= 0) {
                    /* days start at 1, not in correct range */
                    n.commit();
                    dbConn->releaseDBaccess();
                    return DATA_DIR_ERROR;
                }
                dataDir->hour = r[0]["\"hour\""].as<int>();
                dataDir->dbID = r[0]["\"id\""].as<int>();
                dataDir->dirName = upmuFtpAccess::createDirName_YrMonDayHr(dataDir);
                dataDir->state = processed;
                startingDirInDB = true;
            }
        }
        if(!startingDirInDB) {
            /* there is no current uPMU data directory, s propose one */
            dataDir->year = DEFAULT_STARTING_YEAR;
            dataDir->month = DEFAULT_STARTING_MONTH;
            dataDir->day = DEFAULT_STARTING_DAY;
            dataDir->hour = DEFAULT_STARTING_HOUR;
            dataDir->dirName = upmuFtpAccess::createDirName_YrMonDayHr(dataDir);
            dataDir->state = proposed; 
        }  
        /* close transaction */
        n.commit();
        dbConn->releaseDBaccess();

        unsigned int et = upmuFtpAccess::createTimeFromDataDir(dataDir);
        dataDir->epochDay = et / 86400;

        /* loop til most recent directory is found */
        bool termLoop = false;
        bool ftpError = false;
        dataDirValid = false;
        mostRecentDataFileNameValid = false;
        int sts;
        while (!termLoop) {
            /* deal with special case that we have no current dir,
             but have latest processed account.  In that case, we need
             to inc directory before checking for availability. */
            if(dataDir->state == processed) {
                sts = upmuFtpAccess::incDataDir(dataDir);
                    if(sts == DATA_DIR_AT_PRESENT) {
                        dataDirValid = false;
                        termLoop = true;
                        return NO_DATA_DIR;
                   }
            }
            /* is directory valid? */
            sts = upmuFtpAccess::verifyDataDirPresent(dataDir->dirName);
            if(sts == FTP_NOT_RESP) {
                /* ftp error */
                ftpError = true;
                dataDirValid = false;
                termLoop = true;
            }
            else if(sts == DIR_PRESENT) {
                /* dirctory present - are we finished processing it? */
                std::cout << "Directory Present: " << dataDir->dirName << std::endl;
                if(dataDir->state == proposed) {
                    /* save proposed directory in db as current dir */
                    dbConn->acquireDBaccess();
                    work w(*(dbConn->conn));
                    w.prepared("add_dir_record")(dataDir->year)(dataDir->month)\
                        (dataDir->day)(dataDir->hour)((dataDir->dirName).c_str())\
                        ("current").exec();
                        w.commit();
                    /* select it so we can use common code at end of this section.  */
                    nontransaction n(*(dbConn->conn));
                    r = n.exec("SELECT * FROM upmu_data_directory WHERE state = 'current'");
                    n.commit();
                    dataDir->dbID = r[0]["\"id\""].as<int>();
                    dbConn->releaseDBaccess();
                    dataDir->state = current;
                }

                dataDirValid = true;
                termLoop = true;               
            }
            else {
                std::cout << "Directory Not Present: " << dataDir->dirName.c_str() << std::endl;
                if(dataDir->state == current) {
                    /* we need to move to new directory, mark this one as abandoned */
                    dbConn->acquireDBaccess();
                    work w(*(dbConn->conn));
                    w.prepared("abandon_dir_record")(dataDir->dbID).exec();
                    w.commit();
                    dbConn->releaseDBaccess();
                }
                sts = upmuFtpAccess::incDataDir(dataDir);
                if(sts == DATA_DIR_AT_PRESENT) {
                    dataDirValid = false;
                    termLoop = true;
                }

            }
        }
        return DATA_DIR_OK;
    }
    catch (const std::exception &e) {
        /* log error */
        std::cerr << e.what() << std::endl;
        return NO_DATA_DIR;
    }
}

int serviceDataDiscovery::getCurrentDataFileSet() {
    
    try {
        dbConn->acquireDBaccess();
        nontransaction n(*(dbConn->conn));
        result r = n.prepared("search_data_record_by_dir_index")(dataDir->dbID).exec();
        n.commit();  
    
        if(r.size() == 0) {
            /* no data files from this directory have been processed.
            Set scan context variables as needed. Note, data processing sets will
            properly initialized in upmuProcessData(). */
            dataDirAccessInProgress = false;
        }
        else {
            dataFilesInDir.clear();
            dataFilesProcessed.clear();
            dataDirAccessInProgress = true;
                
            for (pqxx::result::const_iterator row = r.begin(); row != r.end(); row++) {       
                /* get important fields into local variables */               
                pqxx::result::field field = row->at("state");
                std::string s = field.as<std::string>();
                if((s == "abandoned") || (s == "released")) {
                    /* we have dealt with this file. */
                    field = row->at("file_name");
                    std::string t = field.as<std::string>();
                    dataFilesProcessed.insert(t);
                }
            }
        }
        dbConn->releaseDBaccess();
        return DATA_DIR_OK;
    }
    catch (const std::exception &e) {
        /* log error */
        dbConn->releaseDBaccess();
    }
    return DATA_DIR_ERROR;
}

int serviceDataDiscovery::addDataFile2DB(upmuFtpAccess * upmuDaq, 
    std::string fileName, const std::string& status) {
    
    return 0;
}

int serviceDataDiscovery::processUPMUdata() {
    if(dataDirValid) {
        if(!dataDirAccessInProgress) {
            dataDirAccessInProgress = true;
            time(&dataDirAccessStartTime);
            dataFilesInDir.clear();
            dataFilesProcessed.clear();
        }
        if(dataFilesInDir.empty()) {
            upmuFtpAccess::getDataFileList(&dataFilesInDir, &dataFilesProcessed,
                &(dataDir->dirName));
            /*******/
        }
        if(!dataFilesInDir.empty()) {
            /* is DataDiscovery service flow controlled? */
            if(sharedInMemDataBufferCnt <= MAX_OUTSTANDING_MEM_BUF) {
                /* process first data file in list */
                std::string dataFile = * dataFilesInDir.begin();
                /* determine if data file is complete or still being filled */
                if(dataFileComplete(&dataFile)) {
                    /* keep a copy for abandon calculation */
                    mostRecentDataFileNameValid =true;
                    mostRecentDataFileName = dataFile;
                    dataFilesProcessed.insert(dataFile);
                    dataFilesInDir.erase(dataFile);
                    try {
                        unsigned char *dataBufAddr;
                        unsigned int fileLen;
                        /* insert record into upmu_data_file table that documents we
                         are processing this data file. */
                        work w(*(dbConn->conn));
                        w.prepared("add_data_record")(dataFile.c_str())(dataDir->dbID)
                            ("processing").exec();
                        w.commit();
                        nontransaction n(*(dbConn->conn));
                        /* get the id for the record we just created. */
                        result r = n.prepared("get_data_record_key").exec();
                        n.commit();
                        int dataRecordID = r[0][0].as<int>();
                        std::cout << "Data File Processed: " << dataFile.c_str() << std::endl;
                        fileLen = upmuFtpAccess::getDataFile(&(dataDir->dirName), 
                            &dataFile, &dataBufAddr);
                        postProcessUPMUdata(dataBufAddr, (unsigned int)fileLen);
                        /* save recent file lengths for future use */
                        mostRecentDataFileLength = (unsigned int)fileLen;
                        /* Mark the data file record just created as released. */
                        work w1(*(dbConn->conn));
                        w1.prepared("release_data_record")(dataRecordID).exec();
                        w1.commit();
                    }
                    catch (const std::exception &e) {
                        /* log error */
                        std::cerr << e.what() << std::endl;
                        return DB_ACCESS_ERROR;
                    }
                }
                else {
                    /* data file incomplete or ftp error.  retry and hope upmu
                     will become available. */
                    //DEBUG: std::cout << "Data File Incomplete. " << std::endl;
                }
            }
        }
        /* are we done with - or need to abandon - this directory? */
        if(dataFilesInDir.empty()) {
            time_t rawt;
            time(&rawt);
            if(!dataDirValid) {
                std::cout << " datDirValid in wrong state." << std::endl;
            }
            else {
                unsigned int dirt = upmuFtpAccess::createTimeFromDataDir(dataDir);
                if(((unsigned int)rawt - dirt) > 60 * 60) {
                    /* directory is over 1 hr. old, abandon it. */
                    dataDirValid = false;
                    dataDirAccessInProgress = false;
                    mostRecentDataFileNameValid = false;
                }
            }
        }      
    }
    if(!dataDirValid) {
        /* just exhausted data files in this directory */
        /*  mark this one as done */
        if(dataDir->state == current) {
            /* just finished a current directory, so let's mark it as closed */
            try {
                work w(*(dbConn->conn));
                w.prepared("release_dir_record")(dataDir->dbID).exec();
                w.commit();
            }
            catch (const std::exception &e) {
                /* log error */
                std::cerr << e.what() << std::endl;
                return DB_ACCESS_ERROR;
            }
            dataDir->state = unknown;
            dataDirAccessInProgress = false;
        }
        
        int sts = upmuFtpAccess::isDataDirAtPresent(dataDir);
        if(sts != DATA_DIR_AT_PRESENT) {
            int sts = upmuFtpAccess::incDataDir(dataDir);
        }
        /* we're just beyond the most recent directory */
        /* wait here until something shows up in uPMU */
        sts = upmuFtpAccess::verifyDataDirPresent(dataDir->dirName);
        if(sts == DIR_PRESENT) {
            addCurrentDirectoryRecord();
            dataDir->state = current;
            dataDirValid = true;
            std::cout << "Directory Present: " << dataDir->dirName << std::endl;          
        }
        else {
            if(sts == FTP_NOT_RESP){
                std::cout << "FTP connection BROKE, waiting 45minutes to continue: " << std::endl;
                std::this_thread::sleep_for(std::chrono::minutes(45));
            }
            else{
            /*********/
            std::cout << "Directory Not Present: " << dataDir->dirName << std::endl;
            }
        }
    }
}

bool serviceDataDiscovery::dataFileComplete(std::string * dataFile) {
    time_t rawt;
    time(&rawt);
    unsigned int dft = upmuFtpAccess::createTimeFromDataFileName(*dataFile);
    if(((unsigned int)rawt - dft) > CATCH_UP_VS_NORMAL_THRESH_SEC) {
        /* data file is sufficiently old, it must be complete */
        scanMode = catch_up;
        serviceState = STATE_CATCH_UP;
        return true;
    }
    else {
        double fileLength;
        scanMode = normal;
        serviceState = STATE_NORMAL;
        int sts = upmuFtpAccess::getDataFileLength(
            &(dataDir->dirName), dataFile, &fileLength);
        if(sts != DATA_ACCESS_OK) {
            return false;
        }
        else {
            unsigned int fileSize = (unsigned int)fileLength;
            if(fileSize < mostRecentDataFileLength) {
                /* not the same size as previous data files, so we wait.
                 Note that if file lengths change in uPMU configure, we will
                 take an earlier branch once its old enough.  */
                return false;
            }
            else {
                return true;
            }
        }
            
    }
}

void serviceDataDiscovery::postProcessUPMUdata(unsigned char *dataBuf,
    unsigned int dataLen) {
    int sts;
    unsigned char * dataPtr = dataBuf;
    
    /* inspect first top bytes of year  */
    bool dataBigEndian = false;
    bool platformBigEndian = IS_BIG_ENDIAN;
    if((*(dataPtr + 4) == 0) && (*(dataPtr + 5) == 0)) {
        dataBigEndian = true;
    }
    if(platformBigEndian != dataBigEndian) {
        /* we need to swap bytes in the buffer */
        while (dataPtr < dataBuf + dataLen) {       
            unsigned char * tempDataPtr = dataPtr;
            /* loop over 1 sec. of data */
            while (tempDataPtr < (dataPtr + 6312)) {
                /* treat everything as if it were 32 bit value */
                swapBytesUint((unsigned int *)tempDataPtr);
                tempDataPtr += 4;
            }
            dataPtr += 6312;
        }
    }
    
    std::shared_ptr<serviceDataBuffer> 
            ptr(new serviceDataBuffer(&sharedInMemDataBufferCnt, 
            &sharedTotalDataBufferCnt, dataLen, dataBuf));
    /* offer data to other services */
    for (auto si: services->services) {
        if(si.second->serviceAcceptsData== true) { //this is better than the old solution
    //    if(si.first.compare("serviceDataDiscovery") != 0) { 
    //       if(si.first.compare("serviceDataArchiver") == 0) {
                dataBufferOffer_t sts =
                    si.second->serviceThread->dataBufferOffer(ptr);
                std::cout << "Discovery Buffer offered, internal counter: " << sharedInMemDataBufferCnt << ", to" << si.first << std::endl;
      //      }
        }
    }
    /* release our reference to the shared pointer.  If any other service is
     using it, they have already added their own reference.  It will be destroyed when the
     last reference is released. */
    ptr.reset();
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

void serviceDataDiscovery::addCurrentDirectoryRecord() {
   
    try {
        work w(*(dbConn->conn));
        w.prepared("add_dir_record")(dataDir->year)(dataDir->month)\
            (dataDir->day)(dataDir->hour)((dataDir->dirName).c_str())\
            ("current").exec();
            w.commit();
        /* select it so we can use common code at end of this section.  */
        nontransaction n(*(dbConn->conn));
        result r = n.prepared("get_dir_record_key").exec();
        n.commit();
        dataDir->dbID = r[0][0].as<int>();
    }
    catch (const std::exception &e) {
        /* log error */
        std::cerr << e.what() << std::endl;      
    }
}

serviceDataDiscovery plugin;
BOOST_DLL_AUTO_ALIAS(plugin)

}
