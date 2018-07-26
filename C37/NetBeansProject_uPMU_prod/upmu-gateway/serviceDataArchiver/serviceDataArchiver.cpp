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
 * Author: mcp & rgentz
 */
#include <iostream>
#include <fstream>
#include <chrono>
#include <time.h>
#include <thread>
#include <atomic>
#include <set>
#include <pqxx/pqxx>
#include "cassandra.h"
#include <boost/make_shared.hpp>
#include <boost/thread.hpp>
#include <boost/ref.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/dll/alias.hpp>
#include <boost/thread/condition.hpp>
#include <boost/any.hpp>
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
#include "serviceDataBuffer.h"

#include "serviceCommandQueueTemplate.h"
#include "serviceInstanceApi.h"
#include "serviceStatus.h"
#include "upmuDataProtobuf.pb.h"
#include "serviceOperationsLoggerPostgres.h"
#include "serviceOperationsLoggerCassandra.h"
#include "upmuStructKeyValueListProtobuf.pb.h"

using namespace serviceCommon;

#include "oneSecDataFormat.h"
#include "serviceCommonStats.h"
#include "serviceSpecificStats.h"
#include "uPMUparticulars.h"
#include "uPMUgmtime.h"

#include "serviceDataArchiver.h"


#include <byteswap.h> //testing


namespace serviceDataArchiver {
using namespace boost;
using namespace std;
using namespace serviceCommon;
using namespace std::chrono;

serviceDataArchiver::serviceDataArchiver() {
    std::cout<< "serviceDataArchiver::Creating serviceDataArchiver" << std::endl;
    cmdQueue = new SynchronisedCommandQueue<SERVICE_COMMAND *>(CMD_QUEUE_LEN);
    m_thread=NULL;
    m_mustStop=false;
    serviceStatus = NOT_STARTED;
}

serviceDataArchiver::~serviceDataArchiver() {
    if (m_thread!=NULL) delete m_thread;
}

// Start the thread 
void serviceDataArchiver::start() {
    // Pass myself as reference since I don't want a copy
    m_mustStop = false;  
    /* start thread.  operator() () is the function executed */
    m_thread=new boost::thread(boost::ref(*this));
}
 
// Stop the thread
void serviceDataArchiver::stop() {
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
 
dataBufferOffer_t serviceDataArchiver::dataBufferOffer(
    const std::shared_ptr<serviceDataBuffer>& offeredBuf) {    
    
    serviceCmnStatsMutex.lock();
    unsigned int * cnt = any_cast<unsigned int *>
        ((*serviceCmnStatsValues)[INDX_UINT_NUM_DATA_BUF_SEEN]);
    *cnt++;
    serviceCmnStatsMutex.unlock();
    
    switch (DATA_QUEUE_LEN - dataQueue.size()) {
        case 0: {
            /* no room for buffer,  return rejected */
            serviceCmnStatsMutex.lock();
            unsigned int * cnt = any_cast<unsigned int *>
                ((*serviceCmnStatsValues)[INDX_UINT_NUM_DATA_BUF_REJECTED]);
            *cnt++;
            serviceCmnStatsMutex.unlock();
            return BUFFER_REFUSED;
        }
        default: {
            /* buffer accepted, ownership transferred */
            m_dataQueueMutex.lock();  
            std::shared_ptr<serviceDataBuffer> ptr = offeredBuf;
            dataQueue.push(ptr);
            m_dataQueueMutex.unlock();
            
            serviceCmnStatsMutex.lock();
            unsigned int * cnt = any_cast<unsigned int *>
                ((*serviceCmnStatsValues)[INDX_UINT_NUM_DATA_BUF_ACCEPTED]);
            *cnt++;
            serviceCmnStatsMutex.unlock();
            return BUFFER_ACCEPTED;
        }
    }
    return BUFFER_ACCEPTED;
}

// Thread function
void serviceDataArchiver::operator () () {
    bool localMustStop = false;
    SERVICE_COMMAND * cmd;
    serviceStatus = RUNNING;
    
    /* create log4cpp related appender objects for this service. */
    logger_serviceDataArchiver.addAppender(logger_appender);
    /* log startup message */
    logger_serviceDataArchiver.info("serviceDataArchiver starting.");
    
    if(!initService()) {
        /* log error and exit*/
        logger_serviceDataArchiver.error("Could not complete initialization, exiting.");
        return;
    }
    
    try {
        /* buffer processing context record */
        std::shared_ptr<serviceDataBuffer> dataBuf = nullptr;
        bool processingBuffer = false;
        
        while(!localMustStop) {
           /* always check for possible command */
            cmd = cmdQueue->Dequeue();
            if(cmd != 0) {
                processCmd(cmd);
            }
            
            /* are we processing a buffer */
            if(processingBuffer) {
                if(dataBuf == nullptr) {
                    /* Error TBD */
                }
                else {
                    bool complete = processData(&bpr, dataBuf);
                    if(complete) {
                        /* process insert timer */
                        if(bpr.numInsertsInAccum > 0) {
                            float avgInsertTime = bpr.accumInsertTime /
                            bpr.numInsertsInAccum;
                            /* insert into stats */
                            serviceSpecStatsMutex.lock();
                            float * minInsertTime = any_cast<float *>
                                ((*serviceSpecStatsValues)[INDX_FLT_MIN_AVG_DATA_INSERT_MSEC]);
                            float * maxInsertTime = any_cast<float *>
                                ((*serviceSpecStatsValues)[INDX_FLT_MAX_AVG_DATA_INSERT_MSEC]);
                            if(*minInsertTime == 0.0) {
                                *minInsertTime = avgInsertTime;
                            }
                            else if(avgInsertTime < *minInsertTime) {
                                *minInsertTime = avgInsertTime;
                            }
                            if(*maxInsertTime == 0.0) {
                                *maxInsertTime = avgInsertTime;
                            }
                            else if(avgInsertTime > *maxInsertTime) {
                                *maxInsertTime = avgInsertTime;
                            }
                            float * ptrAvgInsertTime = any_cast<float *>
                                ((*serviceSpecStatsValues)[INDX_FLT_LAST_AVG_DATA_INSERT_MSEC]);
                            *ptrAvgInsertTime = avgInsertTime;
                            serviceSpecStatsMutex.unlock();
                            
                        }
                        /* release the buffer and move to next */
                        dataBuf.reset();
                        processingBuffer = false;
                    }
                    else {
                        /* set up to handle next record in buffer */
                       
                    }
                }
            }
            /* check to see if any databuffers are queued */
            else if(!dataQueue.empty()) {
                /* get msg buffer from non-empty queue */
                m_dataQueueMutex.lock();
                dataBuf = dataQueue.front();
                dataQueue.pop();
                m_dataQueueMutex.unlock();
                /* preprocess msg buffer to check for simple format errors and
                 compute tho many 1 sec. buffers are present. */
                int sts = preProcessData(&bpr, dataBuf);
                if(sts <= 0) {
                    /* buffer error */
                    serviceSpecStatsMutex.lock();
                    unsigned int * cnt = any_cast<unsigned int *>
                        ((*serviceSpecStatsValues)[INDX_UINT_ABANDONED_BUFFERS]);
                    *cnt++;
                    serviceSpecStatsMutex.unlock();
                    /* release the buffer by reseting the shared pointer   If no
                     one else is using this buffer, it gets deallocated by its
                     destructor */
                    dataBuf.reset();
                }
                else {
                    /* we're processing this buffer. loop over 1 sec. buffers 
                     contained until everything is processed. Processing starts
                     on the next pass through this loop*/
                    processingBuffer = true;
                }
            }
            /* if we are not procesing a buffer, sleep for a bit and then
            any new requests */
             if(!processingBuffer && dataQueue.empty()) {
                 std::this_thread::sleep_for(std::chrono::seconds(SCAN_MODE_NORMAL_DELAY_SEC));
                 
             }
            /* Get the "must stop" state (thread-safe) for inspection at the
             * top of this loop */
            m_mustStopMutex.lock();
            localMustStop=m_mustStop;
            m_mustStopMutex.unlock();
        }
        /* We got a stop request, so exiting function kills thread and
         * stops the service */
        serviceStatus = NORMAL_STOP;
        std::cout<< "Normal stop serviceDataQA::operator" << std::endl;
        return;
    }
    catch (const std::exception &e) {
        serviceStatus = ERROR_STOP;
        std::cout<< "Error stop serviceDataQA::operator" << std::endl;
        return;
    }
};

void serviceDataArchiver::processCmd(SERVICE_COMMAND * cmd) {
    std::cout<< "serviceDataArchiver::processCmd" << std::endl;
    
}

bool serviceDataArchiver::initService() {
    serviceState = "normal";
    
    /* init service data structures */
    createServiceCommonStats(this);
    createServiceSpecificStats(this);
    
    /* get a db connection for local database */
    dbConnection * dbConn = dbConnection::getInstance();
    if(dbConn->conn == NULL) {
        /* log an error and exit. */
        logger_serviceDataArchiver.error("Cannot get valid postgres connector object.");
        return false;
    }
    /* declare uPMUgateway started at this point. Note: startup message always 
     contains sessionID and major/minor version info.  Also, these params are ad hoc
     in the gateway; but are part of standard plugin statistices package in plugins. */
    postgresOperationsLogger->createStringOperationsRecord(dbConn, serviceState, upmuSerialNumber,
        sessionID, serviceName, "Service_Startup_Msg", "Initialized, waiting for input buffers.");
    postgresOperationsLogger->createStringOperationsRecord(dbConn, serviceState, upmuSerialNumber,
        sessionID, serviceName, 
        (*serviceSpecStatsLabels)[INDX_STR_DATA_ARCHIVER_MAJ_VER]->c_str(), 
        any_cast<std::string *>((*serviceSpecStatsValues)[INDX_STR_DATA_ARCHIVER_MAJ_VER])->c_str());
    postgresOperationsLogger->createStringOperationsRecord(dbConn, serviceState, upmuSerialNumber,
        sessionID, serviceName, 
        (*serviceSpecStatsLabels)[INDX_STR_DATA_ARCHIVER_MIN_VER]->c_str(), 
        any_cast<std::string *>((*serviceSpecStatsValues)[INDX_STR_DATA_ARCHIVER_MIN_VER])->c_str());
    
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
                std::cout<< "free buffer dataArchiver fail";
            }
        } 
        else {
            logger_serviceDataArchiver.error("Cannot form serialized protocol buffer, "
                "insufficient memory."); 
        }
    }
   
    /* init local values */
    dataPtr = nullptr;
    
    
    /* check  if cassandra arciving is possible and enabled */
    if(!cassArchivingEnabled || !cassConnected) {
        /* log abandoned buffer */
        logger_serviceDataArchiver.warn("Cassandra access disabled or"
                " not available.  Proceeding anyway."); 
    }
    return true;
}

void serviceDataArchiver::createServiceStartupKeyValueList(upmuStructKeyValueList * kvList) {
    kvList->set_name("Service_Startup_Info");
    uint32_t startTime = (uint32_t)time(NULL);
    kvList->set_timestamp(startTime);
    KeyValueList * list = kvList->add_list();
    list->set_category("ServiceStatus");

    /* add a few key values */
    serviceSpecStatsMutex.lock();
    KeyValue * pair = list->add_element();
    pair->set_key((*serviceSpecStatsLabels)[INDX_STR_DATA_ARCHIVER_MAJ_VER]->c_str()); 
    pair->set_stringval(any_cast<std::string *>
                ((*serviceSpecStatsValues)[INDX_STR_DATA_ARCHIVER_MAJ_VER])->c_str());

    pair = list->add_element();
    pair->set_key((*serviceSpecStatsLabels)[INDX_STR_DATA_ARCHIVER_MIN_VER]->c_str()); 
    pair->set_stringval(any_cast<std::string *>
                ((*serviceSpecStatsValues)[INDX_STR_DATA_ARCHIVER_MIN_VER])->c_str());
    serviceSpecStatsMutex.unlock();

}

int serviceDataArchiver::preProcessData(
    BUFFER_PROCESSING_RECORD * bpr,
    std::shared_ptr<serviceDataBuffer>& dataBuf) {

    std::cout<< "serviceDataArchiver::preProcessData" << std::endl;
    
    /* perform a few checks on data buffer to stay out of trouble */
    if((dataBuf->byteLen) != (SECBLK_LEN_BYTES * 120) + 4) {
        /* buffer does not have full 120 samples.  For now, we discard it */
        serviceSpecStatsMutex.lock();
        unsigned int * cnt = any_cast<unsigned int *>
            ((*serviceSpecStatsValues)[INDX_UINT_BUF_FMT_ERROR]);
        *cnt++;
        serviceSpecStatsMutex.unlock();
        bpr->numRecords = -1;
        return -1;
    }
    /* FIX
     * else if(!cassArchivingEnabled || !cassConnected) {
        // Cassandra is disabled or not connected 
        serviceSpecStatsMutex.lock();
        unsigned int * cnt = any_cast<unsigned int *>
            ((*serviceSpecStatsValues)[INDX_UINT_INSERT_ABANDON_NO_CASSANDRA_CONN]);
        *cnt++;
        serviceSpecStatsMutex.unlock();
        bpr->numRecords = -1;
        return -1;
    }
     */
    else {
        /* for C37 data, we only have one data set per buffer. */
        bpr->numRecords = 1;
        bpr->nextRecord = 0;
        bpr->numInsertsInAccum = 0;
        bpr->accumInsertTime = 0.0;
       
        return 1;
    }
}

bool serviceDataArchiver::processData(BUFFER_PROCESSING_RECORD *bpr,
    std::shared_ptr<serviceDataBuffer>& dataBuf) {
    /* have we processed all the 1 sec. records discovered by the
     * preProcessData() routine?  If so, we're done with this buffer
     * and can release it and wait for next buffer to show up in the queue */
    if(bpr->nextRecord >= bpr->numRecords) {
        return true;
    }
    else {
        /* create a new protobuf object to contain this 1 sec of data */
        upmuData secData;
        
        /* point to time field in c37_Reformat buffer */
        uint32_t * p32 = (uint32_t *)(dataBuf->dataBuffer) + 8+40;
        //float * p32 = (float *)(dataBuf->dataBuffer) + 8;
        
        //std::cout << "data buf: " << dataBuf->dataBuffer << std::endl;
        
        
       uint32_t workpls;
       float * fwork;
       workpls = __bswap_32 (*p32++);
       p32++; //fractional seconds we dont care about      
        
       // uint64_t epoch1 = __bswap_64 (*p32);
//        uint64_t epochTimepointer = reinterpret_cast <uint64_t *> (&epoch1);
        //uint64_t epochTime = *p32++
        //uint64_t epochTime = *epochTimepointer
        uint64_t epochTime = (uint64_t) workpls; //TODO CHECK IF THIS WORKS
        //std::cout << " epochtime found is" << epochTime << std::endl;
        uint64_t epochTimeMsec = epochTime * 1000;
        uint64_t epochDay = epochTime / 86400;
        secData.set_timestamp(epochTime);
        secData.set_sampleintervalmsec(1000./120.);
        secData.set_numsamples(UPMU_SAMPLES_PER_SEC);
        
        
        
        //uint32_t * p31 = (uint32_t *)(dataBuf->dataBuffer) + 16 ;
        //float * p33 = reinterpret_cast <float *>  (p31);
        //for (int i=0; i<12;i++){
        //    std::cout << "recast" << i << " value " << *p33++ << std::endl;
        //}   
        
        //uint32_t * p34 = (uint32_t *)(dataBuf->dataBuffer) + 0;
        uint32_t * p34 = p32;
       // float * f1= reinterpret_cast <float *> (&test1);
        
        
//         for (int i=0; i<12;i++){
//             workpls = __bswap_32 (*p34++);
//             fwork= reinterpret_cast <float *> (&workpls);
//             
//            std::cout << "redo" << i << " value " << *fwork <<  " valas " << workpls<< std::endl;
//        }   
        
        uint32_t * p35 = p32;
        for (int i=0; i<15*3;i++){
            p34--; //just shift the pointer to the correct location (otherwise 3off)
        }
        
        
        /* fill protobuf message w/contents of upmu sync_output struct */
        /* C37_reformat values are in the same order as original code below */
        for(int indx = 0; indx < UPMU_SAMPLES_PER_SEC; indx++) {
            workpls = __bswap_32 (*p34++);
            fwork= reinterpret_cast <float *> (&workpls);  
            float L1mag= *fwork;
            workpls = __bswap_32 (*p34++);
            fwork= reinterpret_cast <float *> (&workpls);  
            float L1ang= *fwork;
            workpls = __bswap_32 (*p34++);
            fwork= reinterpret_cast <float *> (&workpls);  
            float L2mag= *fwork;
            workpls = __bswap_32 (*p34++);
            fwork= reinterpret_cast <float *> (&workpls);  
            float L2ang= *fwork;
            workpls = __bswap_32 (*p34++);
            fwork= reinterpret_cast <float *> (&workpls);  
            float L3mag= *fwork;
            workpls = __bswap_32 (*p34++);
            fwork= reinterpret_cast <float *> (&workpls);  
            float L3ang= *fwork;
            workpls = __bswap_32 (*p34++);
            fwork= reinterpret_cast <float *> (&workpls);  
            float C1mag= *fwork;
            workpls = __bswap_32 (*p34++);
            fwork= reinterpret_cast <float *> (&workpls);  
            float C1ang= *fwork;
            workpls = __bswap_32 (*p34++);
            fwork= reinterpret_cast <float *> (&workpls);  
            float C2mag= *fwork;
            workpls = __bswap_32 (*p34++);
            fwork= reinterpret_cast <float *> (&workpls);  
            float C2ang= *fwork;
            workpls = __bswap_32 (*p34++);
            fwork= reinterpret_cast <float *> (&workpls);  
            float C3mag= *fwork;
            workpls = __bswap_32 (*p34++);
            fwork= reinterpret_cast <float *> (&workpls);  
            float C3ang= *fwork;
            L1ang *=  (180.0/3.141592653589793238463);
            L2ang = L2ang * (180.0/3.141592653589793238463)- L1ang;
            L3ang = L3ang * (180.0/3.141592653589793238463)- L1ang;
            C1ang = C1ang * (180.0/3.141592653589793238463)- L1ang;
            C2ang = C2ang * (180.0/3.141592653589793238463)- L1ang;
            C3ang = C3ang * (180.0/3.141592653589793238463)- L1ang;
            L1ang =0;
//            std::cout << indx << "l1:" << L1mag << std::endl;
//            std::cout << indx<< "l2:" << L2mag << std::endl;
//            std::cout << indx<< "l3:" << L3mag << std::endl;
//            std::cout << indx<< "l1a:" <<L1ang << std::endl;
//            std::cout << indx<< "l2a:" <<L2ang << std::endl;
//            std::cout << indx<< "l3a:" << L3ang << std::endl;
//            std::cout << indx<< "c1:" << C1mag << std::endl;
//            std::cout << indx<< "c2:" << C2mag << std::endl;
//            std::cout << indx<< "c3:" << C3mag << std::endl;
//            std::cout << indx<< "c1a:" << C1ang << std::endl;
//            std::cout << indx<< "c2a:" << C2ang << std::endl;
//            std::cout << indx<< "c3a:" << C3ang << std::endl;
            p34++; //redo2 value 6.79583e-39 valas 4849665
            p34++; //timestamp again
            p34++; //i think fractional value
            
            
            
//            workpls = __bswap_32 (*p35++);
//            float* L1mag= reinterpret_cast <float *> (&workpls);          
//            workpls = __bswap_32 (*p35++);
//            float* L1ang= reinterpret_cast <float *> (&workpls);    
//            workpls = __bswap_32 (*p35++);
//            float* L2mag= reinterpret_cast <float *> (&workpls);
//            workpls = __bswap_32 (*p35++);
//            float* L2ang= reinterpret_cast <float *> (&workpls);    
//            workpls = __bswap_32 (*p35++);
//            float* L3mag= reinterpret_cast <float *> (&workpls);
//            workpls = __bswap_32 (*p35++);
//            float* L3ang= reinterpret_cast <float *> (&workpls);
//            workpls = __bswap_32 (*p35++);
//            float* C1mag= reinterpret_cast <float *> (&workpls);          
//            workpls = __bswap_32 (*p35++);
//            float* C1ang= reinterpret_cast <float *> (&workpls);    
//            workpls = __bswap_32 (*p35++);
//            float* C2mag= reinterpret_cast <float *> (&workpls);
//            workpls = __bswap_32 (*p35++);
//            float* C2ang= reinterpret_cast <float *> (&workpls);    
//            workpls = __bswap_32 (*p35++);
//            float* C3mag= reinterpret_cast <float *> (&workpls);
//            workpls = __bswap_32 (*p35++);
//            float* C3ang= reinterpret_cast <float *> (&workpls);
//            *L1ang *=  (180.0/3.141592653589793238463);
//            *L2ang = *L2ang * (180.0/3.141592653589793238463)- *L1ang;
//            *L3ang = *L3ang * (180.0/3.141592653589793238463)- *L1ang;
//            *C1ang = *C1ang * (180.0/3.141592653589793238463)- *L1ang;
//            *C2ang = *C2ang * (180.0/3.141592653589793238463)- *L1ang;
//            *C3ang = *C3ang * (180.0/3.141592653589793238463)- *L1ang;
//            *L1ang =0;
            
         
            
            
            
            p35++; //redo2 value 6.79583e-39 valas 4849665
            p35++; //timestamp again
            p35++; //i think fractional value
            
            
            syncOutput * so = secData.add_sample();
            //so->set_lockstate(secPtr->lockstate[indx]);
            so->set_lockstate(0); //set it to 0 because we don't have the fild
            so->set_l1mag(L1mag);
            so->set_l1angle(fmod(L1ang, 360.));
            so->set_l2mag(L2mag);
            so->set_l2angle(fmod(L2ang, 360.));
            so->set_l3mag(L3mag);
            so->set_l3angle(fmod(L3ang, 360.));
            so->set_c1mag(C1mag);
            so->set_c1angle(fmod(C1ang, 360.));
            so->set_c2mag(C2mag);
            so->set_c2angle(fmod(C2ang, 360.));
            so->set_c3mag(C3mag);
            so->set_c3angle(fmod(C3ang, 360.));
          
          
          
           
            

            
            
            
           
        }
        
        
        /* check  if cassandra arciving is possible and enabled */
        if(!cassArchivingEnabled || !cassConnected) {
            /* log abandoned buffer */
            serviceSpecStatsMutex.lock();
            unsigned int * cnt = any_cast<unsigned int *>
                ((*serviceSpecStatsValues)[INDX_UINT_INSERT_ABANDON_NO_CASSANDRA_CONN]);
            *cnt++;
            serviceSpecStatsMutex.unlock();
            bpr->nextRecord++; //TODO added by Reinhad not sure if usefull
            return false; //changed from false
        }
        else {
            /* serialize the protobuf message by getting its length, allocating
             * sufficient memory and serializing it to the allocated memory. */
            unsigned int byteLen = secData.ByteSize();
            void * pbBuffer = std::malloc(byteLen);
            if(pbBuffer == nullptr) {
                /* we're out of memory log it.*/
                /* skip the buff.  Hope to catch error at queued message
                buffer level */
                serviceSpecStatsMutex.lock();
                unsigned int * cnt = any_cast<unsigned int *>
                    ((*serviceSpecStatsValues)[INDX_UINT_PB_NO_SERIAL_MEM_AVAIL]);
                *cnt++;
                serviceSpecStatsMutex.unlock();
                bpr->nextRecord++;
                return true; //changed from false
            }       
            secData.SerializeToArray(pbBuffer, byteLen);

            /* archive it */
            
            int sts =  cassandraDataArchive(bpr, epochTimeMsec, epochDay,
                upmuSerialNumber, DOMAIN_DATA, MSG_KEY_VALUE_LIST, pbBuffer, byteLen);
            /* ALWAYS free protobuffer serial storage manually */
            try{
                std::free(pbBuffer);
            }
            catch(...){
                std::cout<<"buffer free failure ServiceDataArchiver";
            }
            
            /* record is archived, so point to next second of data */
            bpr->nextRecord++;
            return false; //changed from false
        }
        return false; //changed from false
    }
}

int serviceDataArchiver::cassandraDataArchive(BUFFER_PROCESSING_RECORD * bpr,
        uint64_t timestampMsec, uint64_t day,
        std::string device, unsigned int domainType, unsigned int msgType,
        void * buffer, unsigned int byteLen) {
        
    const auto begin = high_resolution_clock::now(); // or use steady_clock if high_resolution_clock::is_steady is false
  
    CassStatement * statement = cass_prepared_bind(cassPreparedData);
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
        serviceSpecStatsMutex.lock();
        unsigned int * cnt = any_cast<unsigned int *>
            ((*serviceSpecStatsValues)[INDX_UINT_CASSANDRA_DATA_INSERT_ERR]);
        *cnt++;
        serviceSpecStatsMutex.unlock();
    }

    /* Statement objects can be freed immediately after being executed */
    cass_statement_free(statement);
    cass_future_free(cassQuery_future);
    
    auto time = high_resolution_clock::now() - begin;
    bpr->accumInsertTime += duration<float, std::milli>(time).count();
    bpr->numInsertsInAccum;
    //std::cout << "Elapsed time: " << duration<double, std::milli>(time).count() << ".\n";
    
}

int serviceDataArchiver::cassandraCommonArchive(const CassPrepared * prepared,
        uint32_t statsIndex,
        uint64_t timestampMsec, uint64_t day,
        std::string device, uint32_t domainType, uint32_t msgType,
        void * buffer, uint32_t byteLen) {
     
    CassStatement * statement = cass_prepared_bind(prepared);
    /* Bind the values using the indices of the bind variables */
    /* If we are writing to operations table, add items below */
    if(prepared == cassPreparedOperations) {
        cass_statement_bind_int64_by_name(statement, "SESSION_ID", sessionID);
        cass_statement_bind_string_by_name(statement, "COMPONENT", serviceName.c_str());
    }
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

serviceDataArchiver plugin;
BOOST_DLL_AUTO_ALIAS(plugin)

}
