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

 * Author: mcp and Reinhard Gentz
 * 
 * Created on January 24, 2016, 5:01 PM
 */
//for mac addr
#include <sys/ioctl.h>
#include <net/if.h> 
#include <unistd.h>
#include <netinet/in.h>
#include <string.h>
//for ip addr
#include <stdio.h>
#include <unistd.h>
#include <string.h> /* For strncpy */
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
//
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
#include <SimpleAmqpClient/SimpleAmqpClient.h>
#include <amqp.h>
#include <sstream>
#include <math.h>

#include "dbConnection.h"
#include "serviceCommand.h"
#include "serviceDataBuffer.h"
#include "domainAndMessageTypes.h"
#include "serviceCommandQueueTemplate.h"
#include "serviceInstanceApi.h"
#include "serviceStatus.h"
//#include "upmuDataProtobuf.pb.h"
#include "serviceOperationsLoggerPostgres.h"
#include "serviceOperationsLoggerCassandra.h"
#include "upmuStructKeyValueListProtobuf.pb.h"

using namespace serviceCommon;

#include "serviceCommonStats.h"
#include "serviceSpecificStats.h"
#include "uPMUparticulars.h"
#include "uPMUgmtime.h"

#include "serviceDataArchiverRabbitMQ.h"
#include "upmuDataProtobufRabbitMQ.pb.h"

#include "json.hpp" 


namespace serviceDataArchiverRabbitMQ {
using namespace boost;
using namespace std;
using namespace serviceCommon;
using namespace std::chrono;
using namespace AmqpClient;
using json = nlohmann::json;

#define annotations_summary true
#define unwrapped_phase true
#define annotations_hourly true
#include <limits>
//#include "MahdiP1.h"

int mahdiruns=-7200;

std::vector<std::vector<double> > vol_mag { 
    {1, 0.8, 0.8, 0.8, 0.8, 0.8, 0.8, 0.8, 0.8, 0.8, 1, 1, 1, 0.8, 0.08, 0.08, 0.08, 0.8, 1, 1},	
    {1, 1, 0.08, 0.08, 0.08, 0.8, 0.8, 0.8, 0.8, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1.8, 1.8, 1.8, 0.8, 0.8, 0.8, 0.8, 1, 1.0, 1.0, 1.0, 1.0, 1.0, 1, 1, 1, 1, 1}
};
int last_sample_V[3][3] = { // in this array, the phases sit on the row and sag, interruption, and swell sit on the columns respectively. When a window of data for a certain phase ends with any of these three events, the corresponding element would be equal to 1. 
    {0, 0, 0},
    {0, 0, 0},
    {0, 0, 0}
};
std::vector<double> v_mag [3]; // a segment of voltage phasor mag 
std::vector<double> v_ang [3]; // a segment of voltage phasor ang 
std::vector<double> c_mag [3]; // a segment of current phasor mag 
std::vector<double> c_ang [3]; // a segment of current phasor ang 

int mahdi_debugcounter=0;
const char* args[] = {"phase-a", "phase-b", "phase-c"};
std::vector<std::string> phase(args,args+sizeof(args)/sizeof(args[0]));
int kk=1; // this index is defined to indicate the segment of the data that is read from the data file. 
int size = 120; //window size 6*120
double vnom = 7200;//12.47*pow(10,3)/sqrt(3); //nominal voltage of line to line
#define max_wdw_sag 3 //if we saw a sag for more than max_wdw samples, it will be labeled as a sag. 
#define over 120*60 // this is the number of samples that should be observed before we label something as overvoltage/undervotage/sustained interruption and should be equal to 60 seconds worth of data. 
const double pi=3.14159265358979323846;  /* pi */
struct last_wdw_rep { // this is used to say whether in the last window we didn't report the last event that lasted till the end of the window
    std::vector<double> V {0, 0, 0};
};


struct last_wdw_val { // this is used to save the last value of the quantity in the previous window
    std::vector<double> V {0, 0, 0};
}; 

struct an_count {
    std::vector<double> V_r1 {0, 0, 0};
    std::vector<double> V_r2 {0, 0, 0};
    std::vector<double> V_r3 {0, 0, 0};
    std::vector<double> tmp_V_r1 {0, 0, 0};
    std::vector<double> tmp_V_r2 {0, 0, 0};
    std::vector<double> tmp_V_r3 {0, 0, 0};
}; 
//
struct an_type {
    std::vector<std::string> V[3]; 
};

struct reportme {
    int phase;
    int starttime;
    int endtime;
    int severity;
    std::string description;
};

reportme makereport(int phase, int starttime, int endtime,  int severity, std::string description){
    reportme reportmeobj= {phase,starttime,endtime,severity,description};
    return reportmeobj;
}

struct start_time {
    std::vector<double> V[3];
};
struct end_time {
    std::vector<double> V[3];
};
struct an_ind {
    std::vector<double> V {0, 0, 0};
};


std::vector<reportme> reportp1;

class upmuanalyticsASU {
    public:
        void VoltageLimitMain(std::vector<double> input[3],int phasesofintererst);
        void printReports (std::vector<double> input[3],int phasesofintererst); 
        an_count k;
        start_time ts;
        end_time te;
        an_type an;
        an_ind report;
        last_wdw_rep last_window_rep; 
        last_wdw_val last_window_val; 
        bool found_something;
        
        
};
upmuanalyticsASU analysis;
std::string serviceDataArchiverRabbitMQ::getlocalip(){
        int fd;
    struct ifreq ifr;

    fd = socket(AF_INET, SOCK_DGRAM, 0);

    /* I want to get an IPv4 IP address */
    ifr.ifr_addr.sa_family = AF_INET;

    /* I want an IP address attached to "eth0" */
    strncpy(ifr.ifr_name, "eth0", IFNAMSIZ-1);

    ioctl(fd, SIOCGIFADDR, &ifr);

    close(fd);

    /* Display result */
    printf("%s\n", inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));

    return inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr);
    
}


unsigned char serviceDataArchiverRabbitMQ::getlocalmac(){
    

    unsigned char mac_address[6];

   
    return *mac_address;
}


serviceDataArchiverRabbitMQ::serviceDataArchiverRabbitMQ() {
    std::cout<< "Creating serviceDataArchiverRabbitMQ" << std::endl;
    cmdQueue = new SynchronisedCommandQueue<SERVICE_COMMAND *>(CMD_QUEUE_LEN);
    m_thread=NULL;
    m_mustStop=false;
    serviceStatus = NOT_STARTED;
    // Verify that the version of the library that we linked against is
    // compatible with the version of the headers we compiled against.
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    //get local mac
    localmac=getlocalmac(); //currently not working
    localip=getlocalip();
   // std::cout << "macaddr"<< localmac;
}

serviceDataArchiverRabbitMQ::~serviceDataArchiverRabbitMQ() {
    std::cout<< "serviceDataArchiverRabbitMQ::~serviceDataArchiverRabbitMQ" << std::endl;
    if (m_thread!=NULL) delete m_thread;
}

// Start the thread 
void serviceDataArchiverRabbitMQ::start() {
    std::cout<< "serviceDataArchiverRabbitMQ::start" << std::endl;
    // Pass myself as reference since I don't want a copy
    m_mustStop = false;  
    /* start thread.  operator() () is the function executed */
    m_thread=new boost::thread(boost::ref(*this));
}
 
// Stop the thread
void serviceDataArchiverRabbitMQ::stop() {
    std::cout<< "serviceDataArchiverRabbitMQ::stop" << std::endl;
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

dataBufferOffer_t serviceDataArchiverRabbitMQ::dataBufferOffer(
    const std::shared_ptr<serviceDataBuffer>& offeredBuf) {    
    std::cout<< "serviceDataArchiverRabbitMQ::dataBufferOffer" << std::endl;
    std::cout << "I am here now! Dataqueue is"<< offeredBuf->byteLen << std::endl;//TODO Rainer
    
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
            std::cout << "Rabbit Buffer Refused"<< std::endl;//TODO Rainer
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
void serviceDataArchiverRabbitMQ::operator () () {
    std::cout<< "serviceDataArchiverRabbitMQ::operator" << std::endl;
    
    bool localMustStop = false;
    SERVICE_COMMAND * cmd;
    serviceStatus = RUNNING;
    
    /* create log4cpp related appender objects for this service. */
    logger_serviceDataArchiverRabbitMQ.addAppender(logger_appender);
    /* log startup message */
    logger_serviceDataArchiverRabbitMQ.info("serviceDataArchiver starting.");
    
    if(!initService()) {
        /* log error and exit*/
        logger_serviceDataArchiverRabbitMQ.error("Could not complete initialization, exiting.");
        return;
    }
    
    int testCnt = 0;
    try {
        /* buffer processing context record */
        std::shared_ptr<serviceDataBuffer> dataBuf = nullptr;
        bool processingBuffer = false;
        
        while(!localMustStop) {
            testCnt++;
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

void serviceDataArchiverRabbitMQ::processCmd(SERVICE_COMMAND * cmd) {
    std::cout<< "serviceDataArchiverRabbitMQ::processCmd" << std::endl;
    
}

bool serviceDataArchiverRabbitMQ::initService() {
    serviceState = "normal";
    std::cout<< "serviceDataArchiverRabbitMQ::initService" << std::endl;
    /* init service data structures */
    createServiceCommonStats(this);
    createServiceSpecificStats(this);
    
    /* get a db connection for local database */
    dbConnection * dbConn = dbConnection::getInstance();
    if(dbConn->conn == NULL) {
        /* log an error and exit. */
        logger_serviceDataArchiverRabbitMQ.error("Cannot get valid postgres connector object.");
        return false;
    }
    
    /* get connection info from database and establish conection with
     rabbitMQ server. */
    try {
        dbConn->acquireDBaccess();
        nontransaction nt(*(dbConn->conn));
        result r = nt.exec("SELECT * FROM rabbitmq ORDER BY last_modified DESC LIMIT 1");
        /* should be exactly one row */
        if(r.size() != 1) {
            logger_postgres.error("serviceDtaArchiverRabbitMQ: Multiple or missing rabbitmq config "
                    " records in postgres table: rabbitmq.");       
            return -1;
        }
        /* get important fields into local variables */
        pqxx::result::field field = r[0]["\"routekey\""];
        
        rabbitmqRoutekey = field.c_str();
        rabbitmqRoutekey.append(upmuSerialNumber);
        field = r[0]["\"exchange\""];
        rabbitmqExchange = field.c_str();
        field = r[0]["\"vhost\""];
        rabbitmqVhost = field.c_str();
        field = r[0]["\"db_user\""];
        rabbitmqUser = field.c_str();
        field = r[0]["\"passwd\""];
        rabbitmqPasswd = field.c_str();
        field = r[0]["\"ip_address\""];
        rabbitmqIpAddress = field.c_str();       
        field = r[0]["\"port\""];
        rabbitmqPort = field.as<int>();
        field = r[0]["\"location\""];
        rabbitmqlocation= field.c_str();  
        field = r[0]["\"archiving_enabled\""];
        rabbitmqArchivingEnabled = field.as<bool>();
        
        nt.commit();
        dbConn->releaseDBaccess();
        
        
        
        if(rabbitmqArchivingEnabled) {
            /* set rabbitMQ properties flags */
            rabbitmqProp._flags = 0;
             rabbitmqProp._flags |= AMQP_BASIC_DELIVERY_MODE_FLAG;
             rabbitmqProp.delivery_mode = 2; // persistent delivery mode
             
             /* this is an alternative way to open a rabbitchannel
              char *szBroker = getenv("AMQP_BROKER");
                if (szBroker != NULL)
                    channel2 = Channel::Create(szBroker);
                else
                    channel2 = Channel::Create();
             */
             
             
            /* Setup and connect to cluster */
            try {
                //rabbitmqWriteChannel = Channel::Create(rabbitmqIpAddress.c_str(),
                 //   rabbitmqPort, rabbitmqUser.c_str(), rabbitmqPasswd.c_str(),
                 //   rabbitmqVhost.c_str(), RABBITMQ_MAX_FRAME);
                   rabbitmqWriteChannel = Channel::Create(rabbitmqIpAddress.c_str(),
                    rabbitmqPort, rabbitmqUser.c_str(), rabbitmqPasswd.c_str(),
                    "/", RABBITMQ_MAX_FRAME);
                   //rabbitmqWriteChannel->DeclareQueue(rabbitmqChannel.c_str(),false,false,false,true); //Changed this to be non exclusive, so everyone can subscribe
                  
                logger_rabbitmq.info("serviceDataArchiverRabbitMQ: rabbitMQ "
                    "channel object created.");
                rabbitmqConnected = true;
            }
            catch (std::exception& e) {
                logger_rabbitmq.error("serviceDataArchiverRabbitMQ: could not "
                    " create rabbitMQ channel object.");
                rabbitmqConnected = false;
            }
        }
        else {
            rabbitmqConnected = false;
            logger_rabbitmq.warn("serviceDataArchiverRabbitMQ: rabbitMq access disabled in "
                    "postgres table: rabbitmq.");
        }
        
    }
    catch (const std::exception &e) {
        /* log problem */
        std::string s = "serviceDataArchiverRabbitMQ: exception while "
            "creating rabbitMQ channel object: ";
        s.append(e.what());
        logger_rabbitmq.error(s.c_str());
        rabbitmqConnected = false;  
    }
    
    /* declare uPMUgateway started at this point. Note: startup message always 
     contains sessionID and major/minor version info.  Also, these params are ad hoc
     in the gateway; but are part of standard plugin statistices package in plugins. */
    postgresOperationsLogger->createStringOperationsRecord(dbConn, serviceState, upmuSerialNumber,
        sessionID, serviceName, "Service_Startup_Msg", "Initialized, waiting for input buffers.");
    postgresOperationsLogger->createStringOperationsRecord(dbConn, serviceState, upmuSerialNumber,
        sessionID, serviceName, 
        (*serviceSpecStatsLabels)[INDX_STR_DATA_ARCHIVER_RABBITMQ_MAJ_VER]->c_str(),
        any_cast<std::string *>((*serviceSpecStatsValues)[INDX_STR_DATA_ARCHIVER_RABBITMQ_MAJ_VER])->c_str());
    postgresOperationsLogger->createStringOperationsRecord(dbConn, serviceState, upmuSerialNumber,
        sessionID, serviceName, 
        (*serviceSpecStatsLabels)[INDX_STR_DATA_ARCHIVER_RABBITMQ_MIN_VER]->c_str(), 
        any_cast<std::string *>((*serviceSpecStatsValues)[INDX_STR_DATA_ARCHIVER_RABBITMQ_MIN_VER])->c_str());
    
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
                upmuSerialNumber, 33, 11, pbBuffer, byteLen);
            std::free(pbBuffer);
        } 
        else {
            logger_serviceDataArchiverRabbitMQ.error("Cannot form serialized protocol buffer, "
                "insufficient memory."); 
        }
    }
   
    /* init local values */
    dataPtr = nullptr;
    
    
    /* check  if cassandra arciving is possible and enabled */
    if(!rabbitmqArchivingEnabled || !rabbitmqConnected) {
        /* log abandoned buffer */
        logger_serviceDataArchiverRabbitMQ.warn("Cassandra access disabled or"
                " not available.  Proceeding anyway."); 
    }
    return true;
}

void serviceDataArchiverRabbitMQ::createServiceStartupKeyValueList(upmuStructKeyValueList * kvList) {
    std::cout<< "serviceDataArchiverRabbitMQ::createServiceStartupKeyValueList" << std::endl;
    kvList->set_name("Service_Startup_Info");
    uint32_t startTime = (uint32_t)time(NULL);
    kvList->set_timestamp(startTime);
    KeyValueList * list = kvList->add_list();
    list->set_category("ServiceStatus");

    /* add a few key values */
    serviceSpecStatsMutex.lock();
    KeyValue * pair = list->add_element();
    pair->set_key((*serviceSpecStatsLabels)[INDX_STR_DATA_ARCHIVER_RABBITMQ_MAJ_VER]->c_str()); 
    pair->set_stringval(any_cast<std::string *>
                ((*serviceSpecStatsValues)[INDX_STR_DATA_ARCHIVER_RABBITMQ_MAJ_VER])->c_str());

    pair = list->add_element();
    pair->set_key((*serviceSpecStatsLabels)[INDX_STR_DATA_ARCHIVER_RABBITMQ_MIN_VER]->c_str()); 
    pair->set_stringval(any_cast<std::string *>
                ((*serviceSpecStatsValues)[INDX_STR_DATA_ARCHIVER_RABBITMQ_MIN_VER])->c_str());
    serviceSpecStatsMutex.unlock();

}

int serviceDataArchiverRabbitMQ::preProcessData(
    BUFFER_PROCESSING_RECORD * bpr,
    std::shared_ptr<serviceDataBuffer>& dataBuf) {

    std::cout<< "serviceDataArchiverRabbitMQ::preProcessData" << std::endl;
    
    /* perform a few checks on data buffer to stay out of trouble */
     //std::cout<< "dataBuf->byteLen" << dataBuf->byteLen << std::endl;
       // std::cout<< "UPMU_1SEC_BUF_LEN" << UPMU_1SEC_BUF_LEN << std::endl;
    if((dataBuf->byteLen) % UPMU_1SEC_BUF_LEN != 0) {
        
        std::cout << "Rabbit  buffer does not have integral number of events, too risky to process, discarding data" <<std::endl;
        /* buffer does not have integral number of events, too risky to
         process */
        std::cout<< "dataBuf->byteLen" << dataBuf->byteLen << std::endl;
        std::cout<< "UPMU_1SEC_BUF_LEN" << UPMU_1SEC_BUF_LEN << std::endl;
        serviceSpecStatsMutex.lock();
        unsigned int * cnt = any_cast<unsigned int *>
            ((*serviceSpecStatsValues)[INDX_UINT_BUF_FMT_ERROR]);
        *cnt++;
        serviceSpecStatsMutex.unlock();
        bpr->numRecords = -1;
        return -1;
    }
    else 
//        if(!cassArchivingEnabled || !cassConnected) {
//        /* Cassandra is disabled or not connected */
//        serviceSpecStatsMutex.lock();
//        unsigned int * cnt = any_cast<unsigned int *>
//            ((*serviceSpecStatsValues)[INDX_UINT_INSERT_ABANDON_NO_RABBITMQ_CONN]);
//        *cnt++;
//        serviceSpecStatsMutex.unlock();
//        bpr->numRecords = -1;
//        return -1;
//    }
//    else 
    {
        bpr->numRecords = dataBuf->byteLen / UPMU_1SEC_BUF_LEN;
        bpr->nextRecord = 0;
        bpr->numInsertsInAccum = 0;
        bpr->accumInsertTime = 0.0;
       
        return 1;
    }
}

bool serviceDataArchiverRabbitMQ::processData(BUFFER_PROCESSING_RECORD *bpr,
    std::shared_ptr<serviceDataBuffer>& dataBuf) {
    //std::cout<< "RabbitMQ process data" << std::endl;
     /* have we processed all the 1 sec. records discovered by the
     * preProcessData() routine?  If so, we're done with this buffer
     * and can release it and wait for next buffer to show up in the queue */
    if(bpr->nextRecord >= bpr->numRecords) {
        return true;
    }
    else {
        /* not done yet! */
        sync_output * secPtr = (sync_output *)(dataBuf->dataBuffer
            + bpr->nextRecord * UPMU_1SEC_BUF_LEN);
        /* create a new protobuf object to contain this 1 sec of data */
        //upmuData secData;
        /* create a new protobuf object to contain this 1 sec of data */
        //upmuDataRabbit secDataRabbit;
        /* get an epoch time from uPMU discrete time struct and compute
         both days since epoch and epoch in msec. for later use */
        struct tm t = {0};  // Initalize to all 0's
        t.tm_year = secPtr->sectionTime.year - 1900;  // This is year-1900
        t.tm_mon = secPtr->sectionTime.month - 1;  // jan == 0
        t.tm_mday = secPtr->sectionTime.day;
        t.tm_hour = secPtr->sectionTime.hour;
        t.tm_min = secPtr->sectionTime.min;
        t.tm_sec = secPtr->sectionTime.sec;
        t.tm_zone = "GMT";
        time_t epochTime = _mkgmtime(&t);
        
        uint64_t epochTimeMsec = (uint64_t)epochTime * 1000; //uint needed otherwise overflow
        uint64_t epochDay = epochTime / 86400;
        
//        struct tm  ts;
//        char       buf[80];
//        time(&epochTime);
//        ts = *gmtime(&epochTime);
//        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S %Z", &ts);
//          printf("%s\n", buf);
        
//        secDataRabbit.set_device(upmuSerialNumber);
//        secDataRabbit.set_domaintype(DOMAIN_DATA);
//        secDataRabbit.set_timestampmsec(epochTimeMsec);
//        secDataRabbit.set_msgtype(MSG_KEY_VALUE_LIST);
//        secDataRabbit.set_sampleintervalmsec(secPtr->sampleRate);
//        secDataRabbit.set_numsamples(UPMU_SAMPLES_PER_SEC);
        char bufft[20];
        strftime(bufft, 20, "%Y-%m-%d %H:%M:%S", &t);
        char bufft2[20];
        strftime(bufft2, 20, "%Y-%m-%dT%H:%M:%S", &t);
        
        //secData.set_timestamp(epochTime);
        //secData.set_sampleintervalmsec(secPtr->sampleRate);
        //secData.set_numsamples(UPMU_SAMPLES_PER_SEC);

        /* fill protobuf message w/contents of upmu sylsnc_output struct */
        
        const auto begin = high_resolution_clock::now(); // or use steady_clock if high_resolution_clock::is_steady is false
        
//        mahdi object;
//        object.upmuname= upmuSerialNumber
//        object.sps=UPMU_SAMPLES_PER_SEC;
//        bool important= mahdi::calculate(secPtr, epochtime);
        
        
        float summary_min[3]={std::numeric_limits<float>::infinity(),std::numeric_limits<float>::infinity(),std::numeric_limits<float>::infinity()};
        float summary_max[3]={0};
        double summary_mean[3]={0};
        float summary_min_C[3]={std::numeric_limits<float>::infinity(),std::numeric_limits<float>::infinity(),std::numeric_limits<float>::infinity()};
        float summary_max_C[3]={0};
        double summary_mean_C[3]={0};
        float summary_min_a[3]={std::numeric_limits<float>::infinity(),std::numeric_limits<float>::infinity(),std::numeric_limits<float>::infinity()};
        float summary_max_a[3]={0};
        double summary_mean_a[3]={0};
        float summary_min_C_a[3]={std::numeric_limits<float>::infinity(),std::numeric_limits<float>::infinity(),std::numeric_limits<float>::infinity()};
        float summary_max_C_a[3]={0};
        double summary_mean_C_a[3]={0};
        float temp;
        float unwrap_rabbit[6];
        float unwrap_derivative_min[6]={std::numeric_limits<float>::infinity(),std::numeric_limits<float>::infinity(),std::numeric_limits<float>::infinity()};
        float unwrap_derivative_max[6]={-std::numeric_limits<float>::infinity(),-std::numeric_limits<float>::infinity(),-std::numeric_limits<float>::infinity()};
        double unwrap_derivative_avg[6]={};
        
        
        json j;
        //std::cout<< "first value L1mag is" << secPtr->L1MagAng[0].mag << std::endl;
        for(int indx = 0; indx < UPMU_SAMPLES_PER_SEC; indx++) {
            if(annotations_summary){
                
                summary_min[0]= min({summary_min[0],secPtr->L1MagAng[indx].mag});
                summary_max[0]= max({summary_max[0],secPtr->L1MagAng[indx].mag});
                summary_mean[0] +=secPtr->L1MagAng[indx].mag/UPMU_SAMPLES_PER_SEC;
                summary_min[1]= min({summary_min[1],secPtr->L2MagAng[indx].mag});
                summary_max[1]= max({summary_max[1],secPtr->L2MagAng[indx].mag});
                summary_mean[1] +=secPtr->L2MagAng[indx].mag/UPMU_SAMPLES_PER_SEC;
                summary_min[2]= min({summary_min[2],secPtr->L3MagAng[indx].mag});
                summary_max[2]= max({summary_max[2],secPtr->L3MagAng[indx].mag});
                summary_mean[2] +=secPtr->L3MagAng[indx].mag/UPMU_SAMPLES_PER_SEC;
                summary_min_C[0]= min({summary_min_C[0],secPtr->C1MagAng[indx].mag});
                summary_max_C[0]= max({summary_max_C[0],secPtr->C1MagAng[indx].mag});
                summary_mean_C[0] +=secPtr->C1MagAng[indx].mag/UPMU_SAMPLES_PER_SEC;
                summary_min_C[1]= min({summary_min_C[1],secPtr->C2MagAng[indx].mag});
                summary_max_C[1]= max({summary_max_C[1],secPtr->C2MagAng[indx].mag});
                summary_mean_C[1] +=secPtr->C3MagAng[indx].mag/UPMU_SAMPLES_PER_SEC;
                summary_min_C[2]= min({summary_min_C[2],secPtr->C3MagAng[indx].mag});
                summary_max_C[2]= max({summary_max_C[2],secPtr->C3MagAng[indx].mag});
                summary_mean_C[2] +=secPtr->C3MagAng[indx].mag/UPMU_SAMPLES_PER_SEC;
                temp=fmod(secPtr->L1MagAng[indx].angle,360);
                if (temp<0) temp+=360;
                summary_min_a[0]= min({summary_min_a[0],temp});
                summary_max_a[0]= max({summary_max_a[0],temp});
                summary_mean_a[0] +=temp/UPMU_SAMPLES_PER_SEC;
                temp=fmod(secPtr->L2MagAng[indx].angle,360);
                if (temp<0) temp+=360;
                summary_min_a[1]= min({summary_min_a[1],temp});
                summary_max_a[1]= max({summary_max_a[1],temp});
                summary_mean_a[1] +=temp/UPMU_SAMPLES_PER_SEC;
                 temp=fmod(secPtr->L3MagAng[indx].angle,360);
                 if (temp<0) temp+=360;
                summary_min_a[2]= min({summary_min_a[2],temp});
                summary_max_a[2]= max({summary_max_a[2],temp});
                summary_mean_a[2] +=temp/UPMU_SAMPLES_PER_SEC;
                 temp=fmod(secPtr->C1MagAng[indx].angle,360);
                 if (temp<0) temp+=360;
                summary_min_C_a[0]= min({summary_min_C_a[0],temp});
                summary_max_C_a[0]= max({summary_max_C_a[0],temp});
                summary_mean_C_a[0] +=temp/UPMU_SAMPLES_PER_SEC;
                 temp=fmod(secPtr->C2MagAng[indx].angle,360);
                 if (temp<0) temp+=360;
                summary_min_C_a[1]= min({summary_min_C_a[1],temp});
                summary_max_C_a[1]= max({summary_max_C_a[1],temp});
                summary_mean_C_a[1] +=temp/UPMU_SAMPLES_PER_SEC;
                 temp=fmod(secPtr->C3MagAng[indx].angle,360);
                 if (temp<0) temp+=360;
                summary_min_C_a[2]= min({summary_min_C_a[2],temp});
                summary_max_C_a[2]= max({summary_max_C_a[2],temp});
                summary_mean_C_a[2] +=temp/UPMU_SAMPLES_PER_SEC;
                
            }
            if(unwrapped_phase){
                unwr_tmp_new[0]=fmod(secPtr->L1MagAng[indx].angle,360)+unwr_base[0];
                unwr_tmp_new[1]=fmod(secPtr->L2MagAng[indx].angle,360)+unwr_base[1];
                unwr_tmp_new[2]=fmod(secPtr->L3MagAng[indx].angle,360)+unwr_base[2];   
                unwr_tmp_new[3]=fmod(secPtr->C1MagAng[indx].angle,360)+unwr_base[3];
                unwr_tmp_new[4]=fmod(secPtr->C2MagAng[indx].angle,360)+unwr_base[4];
                unwr_tmp_new[5]=fmod(secPtr->C3MagAng[indx].angle,360)+unwr_base[5];  
                for (int unwrapcounter=0;unwrapcounter<6;unwrapcounter++){
                    if (unwr_tmp_new[unwrapcounter]-unwr_tmp_old[unwrapcounter]>180){
                        unwr_base[unwrapcounter] -=360;
                        unwr_tmp_new[unwrapcounter] -=360;
                        //then we made a jump down

                    }    
                    else {
                        if (unwr_tmp_new[unwrapcounter]-unwr_tmp_old[unwrapcounter]<-180){
                        unwr_base[unwrapcounter] +=360;
                        unwr_tmp_new[unwrapcounter] +=360;
                        //then we made a jump up
                        }
                    }
                    //compute derivative
                    //6 (new-old)+3 (old-veryold)+(veryold-veryveryold)/10*fs/(2pi)
                    float derivative_temp=(6*unwr_tmp_new[unwrapcounter]-3*unwr_tmp_old[unwrapcounter]-2*unwr_tmp_very_old[unwrapcounter]-unwr_tmp_very_old[unwrapcounter])/190.98593171;
                    if (derivative_temp<unwrap_derivative_min[unwrapcounter]){
                         unwrap_derivative_min[unwrapcounter]=derivative_temp;
                    }
                    if (derivative_temp>unwrap_derivative_max[unwrapcounter]){
                         unwrap_derivative_max[unwrapcounter]=derivative_temp;
                    }
                    unwrap_derivative_avg[unwrapcounter] += derivative_temp/UPMU_SAMPLES_PER_SEC;
                   
                    
                    //update storage
                    unwr_tmp_very_old[unwrapcounter]=unwr_tmp_very_old[unwrapcounter];
                    unwr_tmp_very_old[unwrapcounter]=unwr_tmp_old[unwrapcounter];
                    unwr_tmp_old[unwrapcounter]= unwr_tmp_new[unwrapcounter];
                    if (indx==0) {
                        unwrap_rabbit[unwrapcounter]=unwr_tmp_new[unwrapcounter];
                        //std::cout << "debug:unwrap l1= " << unwr_tmp_new[0];
                        }


                }
                
            }
            
            
            //syncOutput * so = secData.add_sample();
//            syncOutputRabbit * so = secDataRabbit.add_sample();
//            
//            so->set_lockstate(secPtr->lockstate[indx]);
//
//            so->set_l1angle(secPtr->L1MagAng[indx].angle);
//            so->set_l1mag(secPtr->L1MagAng[indx].mag);
//
//            so->set_l2angle(secPtr->L2MagAng[indx].angle);
//            so->set_l2mag(secPtr->L2MagAng[indx].mag);
//
//            so->set_l3angle(secPtr->L3MagAng[indx].angle);
//            so->set_l3mag(secPtr->L3MagAng[indx].mag);
//
//            so->set_c1angle(secPtr->C1MagAng[indx].angle);
//            so->set_c1mag(secPtr->C1MagAng[indx].mag);
//
//            so->set_c2angle(secPtr->C2MagAng[indx].angle);
//            so->set_c2mag(secPtr->C2MagAng[indx].mag);
//
//            so->set_c3angle(secPtr->C3MagAng[indx].angle);
//            so->set_c3mag(secPtr->C3MagAng[indx].mag);  
            
            double rabbittime_leftover = double(indx)/UPMU_SAMPLES_PER_SEC;
            std::string rabbittime_leftover_str=std::to_string(rabbittime_leftover);
            rabbittime_leftover_str.erase(0,1); //remove leading 0
                    
            std::string rabbittime = std::string(bufft2)+rabbittime_leftover_str;
            j={
                {"facility", rabbit_facility},
                {"@timestamp", rabbittime},
                {"ANN_AUTHOR", "LBNL_RGentz"},
                {"ANN_VER", "0.1"},
                {"ANN_GROUP", "Standard"} ,
                {"ANN_NAME", "uPMU_raw"},
                {"ANN_TYPE", "BL"},
                {"ANN_SUBCLASS", "1"}, 
                {"ANN_DESC", "Raw Samples"},
                {"ANN_DATASOURCE_UUID", upmuSerialNumber}, 
                {"ANN_COMPUTATION_SITE", localip}, 
                {"ANN_SENSOR_LOCATION", rabbitmqlocation},
                {"ANN_Severity", 0}, 
                {"ANN_PHASE",0},
                {"ANN_TIME_INTERVAL", { 
                   {"STARTTIME_UTC", rabbittime},
                   {"ENDTIME_UTC", rabbittime}
                }},
                {"data", {
                    {"L1Mag",secPtr->L1MagAng[indx].mag},
                    {"L1Ang",fmod(secPtr->L1MagAng[indx].angle,360)},
                    {"L2Mag",secPtr->L2MagAng[indx].mag},
                    {"L2Ang",fmod(secPtr->L2MagAng[indx].angle,360)},
                    {"L3Mag",secPtr->L3MagAng[indx].mag},
                    {"L3Ang",fmod(secPtr->L3MagAng[indx].angle,360)},
                    {"C1Mag",secPtr->C1MagAng[indx].mag},
                    {"C1Ang",fmod(secPtr->C1MagAng[indx].angle,360)},
                    {"C2Mag",secPtr->C2MagAng[indx].mag},
                    {"C2Ang",fmod(secPtr->C2MagAng[indx].angle,360)},
                    {"C3Mag",secPtr->C3MagAng[indx].mag},
                    {"C3Ang",fmod(secPtr->C3MagAng[indx].angle,360)},
                    {"lockState",secPtr->lockstate[indx]}
                }},   
            };
            
            //{"system", upmuSerialNumber},
                //{"type","upmu"},
                //{"host", rabbit_host},
                //{"meter",rabbit_meter},
                //{"poller", rabbit_poller}
            
           // std::cout << j.dump()<<std::endl;
            rabbitmqMsg = BasicMessage::Create(j.dump());    
            std::string raw_exhange="ha-annotations";
            std::string raw_routing_key="raw.ann.upmu."+upmuSerialNumber+"."+std::to_string(indx);
            
            
            try{
            rabbitmqWriteChannel->BasicPublish(raw_exhange.c_str(),raw_routing_key.c_str(),
            rabbitmqMsg, false, false);
            }
            catch(...){
                std::cerr<<"rabbit raw error \n"; 
            }
            
            //Mahdi add data
            v_mag[0].push_back(secPtr->L1MagAng[indx].mag/vnom);
            v_mag[1].push_back(secPtr->L2MagAng[indx].mag/vnom);
            v_mag[2].push_back(secPtr->L3MagAng[indx].mag/vnom);
            v_ang[0].push_back(secPtr->L1MagAng[indx].angle);
            v_ang[1].push_back(secPtr->L2MagAng[indx].angle);
            v_ang[2].push_back(secPtr->L3MagAng[indx].angle);
            c_mag[0].push_back(secPtr->C1MagAng[indx].mag);
            c_mag[1].push_back(secPtr->C2MagAng[indx].mag);
            c_mag[2].push_back(secPtr->C3MagAng[indx].mag);
            c_ang[0].push_back(secPtr->C1MagAng[indx].angle);
            c_ang[1].push_back(secPtr->C2MagAng[indx].angle);
            c_ang[2].push_back(secPtr->C3MagAng[indx].angle);
              
            
        }
         //  gittext              
        if(annotations_summary){
            //std::cout << "making summary now"<<std::endl;

            //std::cout << "Annotations v0.2";
             //std::cout<<std::string(bufft2)+"Z" << std::endl;
            json annotation = {
                {"@timestamp",std::string(bufft2)+"Z"},
                {"ANN_AUTHOR", "LBNL_RGentz"},
                {"ANN_VER", "0.3"},
                {"ANN_GROUP", "Standard"} ,
                {"ANN_NAME", "uPMU_Summary"},
                {"ANN_TYPE", "BL"},
                {"ANN_SUBCLASS", "1"}, 
                {"ANN_DESC", "uPMU, min/max/mean"},
                {"ANN_DATASOURCE_UUID", upmuSerialNumber}, 
                {"ANN_COMPUTATION_SITE", localip}, 
                {"ANN_SENSOR_LOCATION", rabbitmqlocation},
                {"ANN_Severity", 0}, 
                {"ANN_PHASE",0},
                {"ANN_TIME_INTERVAL", { 
                   {"STARTTIME_UTC", epochTimeMsec*1000},
                   {"ENDTIME_UTC", (epochTimeMsec+1000)*1000}}
                },
                {"elastic_index", "upmu"},
                {"elastic_type", "summary"},
                {"ANN_MIN_L",{
                    {"L1magmin", summary_min[0]},
                    {"L2magmin", summary_min[1]},
                    {"L3magmin", summary_min[2]},
                    {"L1angmin", summary_min_a[0]},
                    {"L2angmin", summary_min_a[1]},
                    {"L3angmin", summary_min_a[2]}
                }},
                {"ANN_MAX_L",{
                    {"L1magmax", summary_max[0]},
                    {"L2magmax", summary_max[1]},
                    {"L3magmax", summary_max[2]},
                    {"L1angmax", summary_max_a[0]},
                    {"L2angmax", summary_max_a[1]},
                    {"L3angmax", summary_max_a[2]}
                }},
                {"ANN_MEAN_L",{
                    {"L1magmean", summary_mean[0]},
                    {"L2magmean", summary_mean[1]},
                    {"L3magmean", summary_mean[2]},
                    {"L1angmean", summary_mean_a[0]},
                    {"L2angmean", summary_mean_a[1]},
                    {"L3angmean", summary_mean_a[2]}
                }},
                {"ANN_MIN_C",{
                    {"C1magmin", summary_min_C[0]},
                    {"C2magmin", summary_min_C[1]},
                    {"C3magmin", summary_min_C[2]},
                    {"C1angmin", summary_min_C_a[0]},
                    {"C2angmin", summary_min_C_a[1]},
                    {"C3angmin", summary_min_C_a[2]}
                }},
                {"ANN_MAX_C",{
                    {"C1magmax", summary_max_C[0]},
                    {"C2magmax", summary_max_C[1]},
                    {"C3magmax", summary_max_C[2]},
                    {"C1angmax", summary_max_C_a[0]},
                    {"C2angmax", summary_max_C_a[1]},
                    {"C3angmax", summary_max_C_a[2]}
                }},
                {"ANN_MEAN_C",{
                    {"C1magmean", summary_mean_C[0]},
                    {"C2magmean", summary_mean_C[1]},
                    {"C3magmean", summary_mean_C[2]},
                    {"C1angmean", summary_mean_C_a[0]},
                    {"C2angmean", summary_mean_C_a[1]},
                    {"C3angmean", summary_mean_C_a[2]}
                }},
                
            };
            //std::cout << annotation.dump()<<std::endl;
            rabbitmqMsg = BasicMessage::Create(annotation.dump());
            std::string annotations_routing_key ="summary.ann.upmu."+upmuSerialNumber;
            rabbitmqDataArchive((const char*)"ha-annotations",annotations_routing_key.c_str());
        }
        if(unwrapped_phase){
            //std::cout << "making unwrap now"<<std::endl;

            //std::cout << "Annotations v0.2";
             //std::cout<<std::string(bufft2)+"Z" << std::endl;
            json annotation = {
                {"@timestamp",std::string(bufft2)+"Z"},
                {"ANN_AUTHOR", "LBNL_RGentz"},
                {"ANN_VER", "0.1"},
                {"ANN_GROUP", "Standard"} ,
                {"ANN_NAME", "uPMU_unwrapped_phase"},
                {"ANN_TYPE", "BL"},
                {"ANN_SUBCLASS", "1"}, 
                {"ANN_DESC", "uPMU, unwrapped_phase"},
                {"ANN_DATASOURCE_UUID", upmuSerialNumber}, 
                {"ANN_COMPUTATION_SITE", localip}, 
                {"ANN_SENSOR_LOCATION", rabbitmqlocation},
                {"ANN_Severity", 0}, 
                {"ANN_PHASE",0},
                {"ANN_TIME_INTERVAL", { 
                   {"STARTTIME_UTC", epochTimeMsec*1000},
                   {"ENDTIME_UTC", (epochTimeMsec+1000)*1000}}
                },
                {"elastic_index", "upmu"},
                {"elastic_type", "unwrapped_phase"},
                {"ANN_UNWRAP_L1", {
                    {"unwrap",unwrap_rabbit[1]},
                    {"derivative_min",unwrap_derivative_min[1]},
                    {"derivative_max",unwrap_derivative_max[1]},
                    {"derivative_avg",unwrap_derivative_avg[1]},
                }},
                {"ANN_UNWRAP_L2", {
                    {"unwrap",unwrap_rabbit[2]},
                    {"derivative_min",unwrap_derivative_min[2]},
                    {"derivative_max",unwrap_derivative_max[2]},
                    {"derivative_avg",unwrap_derivative_avg[2]},
                }},
                {"ANN_UNWRAP_L3", {
                    {"unwrap",unwrap_rabbit[3]},
                    {"derivative_min",unwrap_derivative_min[3]},
                    {"derivative_max",unwrap_derivative_max[3]},
                    {"derivative_avg",unwrap_derivative_avg[3]},
                }},
                {"ANN_UNWRAP_C1", {
                    {"unwrap",unwrap_rabbit[4]},
                    {"derivative_min",unwrap_derivative_min[4]},
                    {"derivative_max",unwrap_derivative_max[4]},
                    {"derivative_avg",unwrap_derivative_avg[4]},
                }},
                {"ANN_UNWRAP_C2", {
                    {"unwrap",unwrap_rabbit[5]},
                    {"derivative_min",unwrap_derivative_min[5]},
                    {"derivative_max",unwrap_derivative_max[5]},
                    {"derivative_avg",unwrap_derivative_avg[5]},
                }},
                {"ANN_UNWRAP_C3", {
                    {"unwrap",unwrap_rabbit[6]},
                    {"derivative_min",unwrap_derivative_min[6]},
                    {"derivative_max",unwrap_derivative_max[6]},
                    {"derivative_avg",unwrap_derivative_avg[6]},
                }},    
            };
            //std::cout << annotation.dump()<<std::endl;
            rabbitmqMsg = BasicMessage::Create(annotation.dump());
            std::string annotations_routing_key ="unwrap.ann.upmu."+upmuSerialNumber;
            //rabbitmqDataArchive((,annotations_routing_key.c_str());
            try {
        

       rabbitmqWriteChannel->BasicPublish((const char*)"ha-annotations",annotations_routing_key,
         rabbitmqMsg, true, false);
       //std::cout<< "message published successfully rainer_ex" << std::endl;
       
       

        }

        catch (MessageReturnedException &e)
        {
            std::cout << "Message got returned: " << e.what();
            std::cout << "\nMessage body: " << e.message()->Body();
            logger_serviceDataArchiverRabbitMQ.error(e.message()->Body().c_str());
        }
        catch(std::exception& e) {

            std::string s("Error publishing data protocol"
                " buffer to rabbitMQ server. Exception: ");
            s.append(e.what());

         }
        }
        
        
        if(annotations_hourly && t.tm_hour !=old_hour){
            if(old_hour==0){
                //startup value
            }
            else{
            
            //hourly 
            //std::cout << "making summary now"<<std::endl;

            //std::cout << "Annotations v0.2";
             //std::cout<<std::string(bufft2)+"Z" << std::endl;
            json annotation = {
                {"@timestamp",std::string(bufft2)+"Z"},
                {"ANN_AUTHOR", "LBNL_RGentz"},
                {"ANN_VER", "0.3"},
                {"ANN_GROUP", "Standard"} ,
                {"ANN_NAME", "uPMU_hourly"},
                {"ANN_TYPE", "BL"},
                {"ANN_SUBCLASS", "1"}, 
                {"ANN_DESC", "uPMU, hourly"},
                {"ANN_DATASOURCE_UUID", upmuSerialNumber}, 
                {"ANN_COMPUTATION_SITE", localip}, 
                {"ANN_SENSOR_LOCATION", rabbitmqlocation},
                {"ANN_Severity", 0}, 
                {"ANN_PHASE",0},
                {"ANN_TIME_INTERVAL", { 
                   {"STARTTIME_UTC", epochTimeMsec*1000},
                   {"ENDTIME_UTC", (epochTimeMsec+1000)*1000}}
                },
                {"elastic_index", "upmu"},
                {"elastic_type", "hourly"},
                {"ANN_HOURLY", std::to_string(t.tm_hour)}
                
            };
            //std::cout << annotation.dump()<<std::endl;
            rabbitmqMsg = BasicMessage::Create(annotation.dump());
            std::string annotations_routing_key ="hourly.ann.upmu."+upmuSerialNumber;
            rabbitmqDataArchive((const char*)"ha-annotations",annotations_routing_key.c_str());
            }
            old_hour=t.tm_hour;
        }
        
////// Mahdis stuff //////
        kk++; //mahdi's run counter
        analysis.VoltageLimitMain(v_mag,3); //last one is the phases of interest.
        
        //std::cout<< "mahdis code was called, counter: " << mahdi_debugcounter << std::endl;
        //mahdi_debugcounter++;
//    
//        for(int i=0;i<3;i++){
//            for (auto j =  v_mag[i].begin(); j !=  v_mag[i].end(); ++i)
//                std::cout << *j << ' ';
//        }
        
        if( !reportp1.empty()){
            //std::cout<< "mahdis code has a report"<< std::endl;
            for(auto &reportp1element : reportp1){
                std::cout<< "mahdistart" << reportp1element.starttime << " mahdiend "<< reportp1element.endtime <<"mahdiruns"<<mahdiruns<< std::endl ;
                uint64_t startp1=(epochTimeMsec+uint64_t(float(reportp1element.starttime-mahdiruns)/float(UPMU_SAMPLES_PER_SEC/1000)))*1000;
                uint64_t endp1=(epochTimeMsec+uint64_t(float(reportp1element.endtime-mahdiruns)/float(UPMU_SAMPLES_PER_SEC/1000)))*1000;
                std::cout<<"send MahdiP1 annotation, starttime " <<startp1 << "endtime "<< endp1<< std::endl;
                json mahdiP1 = {
                    {"@timestamp",std::string(bufft2)+"Z"},
                    {"ANN_AUTHOR", "ASU_MJAMEI_LBNL_RGentz"},
                    {"ANN_VER", "0.1"},
                    {"ANN_GROUP", "Standard"} ,
                    {"ANN_NAME", "VoltageLimit"},
                    {"ANN_TYPE", "SP"},
                    {"ANN_SUBCLASS", "1"}, 
                    {"ANN_DESC", reportp1element.description},
                    {"ANN_DATASOURCE_UUID", upmuSerialNumber}, 
                    {"ANN_COMPUTATION_SITE", localip}, 
                    {"ANN_SENSOR_LOCATION", rabbitmqlocation},
                    {"ANN_Severity", reportp1element.severity}, 
                    {"ANN_PHASE",reportp1element.phase},
                    {"ANN_TIME_INTERVAL", { 
                       {"STARTTIME_UTC", startp1},
                       {"ENDTIME_UTC", endp1}
                    }},
                    {"elastic_index", "upmu"},
                    {"elastic_type", "voltage_limit"},   
            };
            
            rabbitmqMsg = BasicMessage::Create(mahdiP1.dump());
            std::string annotations_routing_key ="voltage_limit.ann.upmu."+upmuSerialNumber;
            rabbitmqDataArchive((const char*)"ha-annotations",annotations_routing_key.c_str());
            }
        }
        
        if( t.tm_hour !=old_hour){
            if(old_hour==0){
                //startup value
            }
            else{
                json mahdiP1 = {
                    {"@timestamp",std::string(bufft2)+"Z"},
                    {"ANN_AUTHOR", "ASU_MJAMEI_LBNL_RGentz"},
                    {"ANN_VER", "0.1"},
                    {"ANN_GROUP", "Standard"} ,
                    {"ANN_NAME", "VoltageLimit"},
                    {"ANN_TYPE", "SP"},
                    {"ANN_SUBCLASS", "1"}, 
                    {"ANN_DESC", "heartbeat"},
                    {"ANN_DATASOURCE_UUID", upmuSerialNumber}, 
                    {"ANN_COMPUTATION_SITE", localip}, 
                    {"ANN_SENSOR_LOCATION", rabbitmqlocation},
                    {"ANN_Severity", 0}, 
                    {"ANN_PHASE",0},
                    {"ANN_TIME_INTERVAL", { 
                       {"STARTTIME_UTC", (epochTimeMsec*1000)},
                       {"ENDTIME_UTC", (epochTimeMsec+1000)*1000}
                    }},
                    {"elastic_index", "upmu"},
                    {"elastic_type", "voltage_limit"},   
            };
            
            rabbitmqMsg = BasicMessage::Create(mahdiP1.dump());
            std::string annotations_routing_key ="voltage_limit.ann.upmu."+upmuSerialNumber;
            rabbitmqDataArchive((const char*)"ha-annotations",annotations_routing_key.c_str());
            }
                
        }
                
        
        
        
        mahdiruns+=120;
        reportp1.clear();
        //Clear the v_mag for next usage.
        for (auto& v : v_mag) {
            v.clear();
         }
        for (auto& v : v_ang) {
            v.clear();
         }
        for (auto& v : c_mag) {
            v.clear();
         }
        for (auto& v : c_ang) {
            v.clear();
         }
       

        
//////End Mahdis stuff //////      
        
        
      auto time = high_resolution_clock::now() - begin;
        bpr->accumInsertTime += duration<float, std::milli>(time).count();
        bpr->numInsertsInAccum;
        //std::cout << "Elapsed time: " << duration<double, std::milli>(time).count() << ".\n";
        
        /* check  if cassandra arciving is possible and enabled */
        if(!cassArchivingEnabled || !cassConnected) { //this if condition does not make any sense Rainer TODO
            /* log abandoned buffer */
            serviceSpecStatsMutex.lock();
            unsigned int * cnt = any_cast<unsigned int *>
                ((*serviceSpecStatsValues)[INDX_UINT_INSERT_ABANDON_NO_RABBITMQ_CONN]);
            *cnt++;
            serviceSpecStatsMutex.unlock();
            return false;
        }
        else {
            /* serialize the protobuf message by getting its length, allocating
             * sufficient memory and serializing it to the allocated memory. */
            //unsigned int byteLen = secDataRabbit.ByteSize();
            //void * pbBuffer = std::malloc(byteLen);
//            if(pbBuffer == nullptr) {
//                /* we're out of memory log it.*/
//                /* skip the buff.  Hope to catch error at queued message
//                buffer level */
//                serviceSpecStatsMutex.lock();
//                unsigned int * cnt = any_cast<unsigned int *>
//                    ((*serviceSpecStatsValues)[INDX_UINT_PB_NO_SERIAL_MEM_AVAIL]);
//                *cnt++;
//                serviceSpecStatsMutex.unlock();
//                bpr->nextRecord++;
//                logger_serviceDataArchiverRabbitMQ.error("serviceRabbitMQ: pbBuffer == nullptr, out of memory ");
//                return false;
//            }       
//           // secDataRabbit.SerializeToArray(pbBuffer, byteLen);
//
//            /* archive it */
//            unsigned int domainType = 3;
//            unsigned int msgType = 1;

            /* record is archived, so point to next second of data */
            bpr->nextRecord++;
            
            
            return false;
        }
        return false;
    }
}


    int serviceDataArchiverRabbitMQ::rabbitmqDataArchive(const char* exchange, const char* routekey) {       
//    const auto begin = high_resolution_clock::now(); // or use steady_clock if high_resolution_clock::is_steady is false
      
    try {
        
        //channel2->BasicConsume(rabbitmqChannel.c_str(), "consumer_tag1");
        //BasicMessage::ptr_t the_message = BasicMessage::Create("Body Content");
        
        //channel2->BasicPublish(rabbitmqRoutekey.c_str(), rabbitmqChannel.c_str(), the_message, true, false);
       rabbitmqWriteChannel->BasicPublish(exchange,routekey,
         rabbitmqMsg, true, false);
       
       
      // rabbitmqWriteChannel->BasicPublish(rabbitmqExchange.c_str(),rabbitmqRoutekey.c_str(),
       //  rabbitmqMsg, true, false);
       
       
//       rabbitmqWriteChannel->BasicPublish(exchange,route,message,mandatory,immediate)
        ///for (int i = 0; i < 3; ++i)
        //{
        //    Envelope::ptr_t env;
        //    if (channel2->BasicConsumeMessage("consumer_tag1", env, 0))
        //    {
        //        auto output =env->Message()->Body();
          //      std::cout << std::hex << "Received message with body: " << output << std::endl;
               
                
         //   }
        //}
    }
    //    rabbitmqWriteChannel->BasicPublish(rabbitmqRoutekey.c_str(),
    //        rabbitmqChannel.c_str(), rabbitmqMsg, true, false);
    //}
    catch (MessageReturnedException &e)
    {
        std::cout << "Message got returned: " << e.what();
        std::cout << "\nMessage body: " << e.message()->Body();
        logger_serviceDataArchiverRabbitMQ.error(e.message()->Body().c_str());
    }
    catch(std::exception& e) {
        /* BasicPubish frees buffer.  If we had a fault, it may
         not have been freed.  So, lets try it safely */
        
        serviceSpecStatsMutex.lock();
        unsigned int * cnt = any_cast<unsigned int *>
            ((*serviceSpecStatsValues)[INDX_UINT_RABBITMQ_DATA_INSERT_ERR]);
        *cnt++;
        serviceSpecStatsMutex.unlock();
        
        std::string s("Error publishing data protocol"
            " buffer to rabbitMQ server. Exception: ");
        /* count failures */
        s.append(e.what());
        logger_serviceDataArchiverRabbitMQ.error(s.c_str());
        
        
        //Rabbitreconnect
        if (rabbitreconnectcounter==60){
            rabbitreconnectcounter=0;
            try{
                std::string s("Trying to reconnect rabbitmq ");
                logger_serviceDataArchiverRabbitMQ.error(s.c_str());
                rabbitmqWriteChannel = Channel::Create(rabbitmqIpAddress.c_str(),
                        rabbitmqPort, rabbitmqUser.c_str(), rabbitmqPasswd.c_str(),
                        "/", RABBITMQ_MAX_FRAME);
                s ="Successfully reconnected rabbitmq ";
                logger_serviceDataArchiverRabbitMQ.error(s.c_str());
            }
            catch(...){
                std::string s("Rabbitmq reconnect failed");
                logger_serviceDataArchiverRabbitMQ.error(s.c_str());
            }
        }
        rabbitreconnectcounter++;
        
        try {
            std::free(rabbitmqPkt.bytes); //TODO change this rainer
        }
        catch (std::exception& e) {
            /* nothing to do, but we'll log it. */
            std::string s("Error freeing protocol buffer within fault. "
                "Exception: ");
            s.append(e.what());
            logger_serviceDataArchiverRabbitMQ.error(s.c_str());
        }
    }
    
//    auto time = high_resolution_clock::now() - begin;
//    bpr->accumInsertTime += duration<float, std::milli>(time).count();
//    bpr->numInsertsInAccum;
    //std::cout << "Elapsed time: " << duration<double, std::milli>(time).count() << ".\n";
    
}

int serviceDataArchiverRabbitMQ::cassandraCommonArchive(const CassPrepared * prepared,
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

serviceDataArchiverRabbitMQ plugin;
BOOST_DLL_AUTO_ALIAS(plugin)

        
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <math.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <complex.h>
#include <algorithm>
     
   //     
        

void upmuanalyticsASU::VoltageLimitMain(std::vector<double> input[3],int phasesofintererst) { // this function does the analysis to find the events in the voltage magnitude for a data size defined in size parameter and calls a function print function  
    int Len=input[0].size();
    for (int aa = 0; aa<phasesofintererst; aa++) {
        if (input[aa][0] > 0.9 || input[aa][0] < 0.1)
            k.V_r1[aa] = 0;
        if (input[aa][0] >= 0.1)
            k.V_r2[aa] = 0;
        if (input[aa][0] < 1.1)
            k.V_r3[aa] = 0;
        report.V[aa] = 0;
        int jj = 0;
        while (jj<Len) {
            if (0.1 <= input[aa][jj] && input[aa][jj]<= 0.9) {
                while (0.1 <= input[aa][jj] && input[aa][jj] <= 0.9) {
                    k.V_r1[aa] += 1;
                    if (k.V_r1[aa] > over) {
                        break;
                    }
                    jj += 1;
                    if (jj >= Len) {
                        break;
                    }
                }
                if (k.V_r1[aa] > over) {
                    ts.V[aa].push_back((kk-2)*size+jj-k.V_r1[aa]-1);
                    an.V[aa].push_back("under voltage");
                    te.V[aa].push_back((kk-2)*size+jj);
                    report.V[aa] += 1;                    
                } else if (k.V_r1[aa] > max_wdw_sag) {
                    if(jj-k.V_r1[aa]+k.tmp_V_r1[aa] == 0 && last_sample_V[aa][0] == 1 && k.tmp_V_r1[aa] > max_wdw_sag){ //if a sag starts from beginning of a window and we had also a sag at the end of the last window, they are same event.
                        ts.V[aa].pop_back();                                                                       // also, we only need to do this if the sag in the last window was large enough to be labeled. 
                        an.V[aa].pop_back();
                        te.V[aa].pop_back();
                    } 
                    ts.V[aa].push_back((kk-2)*size+jj-k.V_r1[aa]-1);
                    an.V[aa].push_back("voltage sag");
                    te.V[aa].push_back((kk-2)*size+jj);
                    report.V[aa] += 1; 
                }
                if (jj == Len && k.V_r1[aa] > 0 &&  k.V_r1[aa] <= over){// if the last event in the window was sag, flag the last sample. 
                    last_sample_V[aa][0] = 1;
                    last_sample_V[aa][1] = 0;
                    last_sample_V[aa][2] = 0;
                    k.tmp_V_r1[aa] = k.V_r1[aa];
                } else
                    k.V_r1[aa] = 0;
            } else if (input[aa][jj] < 0.1) {
                while (input[aa][jj] < 0.1) {
                    k.V_r2[aa] += 1;
                    if (k.V_r2[aa] > over) {
                        break;
                    }
                    jj += 1;
                    if (jj >= Len) {
                        break;
                    }  
                }
                if (k.V_r2[aa] > over) {
                    ts.V[aa].push_back((kk-2)*size+jj-k.V_r2[aa]-1);
                    an.V[aa].push_back("sustained interruption");
                    te.V[aa].push_back((kk-2)*size+jj);
                    report.V[aa] += 1;                     
                } else {
                    if(jj-k.V_r2[aa]+k.tmp_V_r2[aa] == 0 && last_sample_V[aa][1] == 1){ //if an interruption starts from begining of a window and we had also an interruption at the end of the last window, they are same event.
                        ts.V[aa].pop_back();                                                                      
                        an.V[aa].pop_back();
                        te.V[aa].pop_back();
                    } 
                    ts.V[aa].push_back((kk-2)*size+jj-k.V_r2[aa]-1);
                    an.V[aa].push_back("interruption");
                    te.V[aa].push_back((kk-2)*size+jj); 
                    report.V[aa] += 1; 
                }
                if (jj == Len && k.V_r2[aa] > 0 &&  k.V_r2[aa] <= over){// if the last event in the window was interruption, flag the last sample. 
                    last_sample_V[aa][1] = 1;
                    last_sample_V[aa][0] = 0;
                    last_sample_V[aa][2] = 0;
                    k.tmp_V_r2[aa] = k.V_r2[aa];
                } else
                    k.V_r2[aa] = 0;
            } else if (input[aa][jj] >= 1.1) {
                while (input[aa][jj] >= 1.1) {
                    k.V_r3[aa] += 1;
                    if (k.V_r3[aa] > over) {
                        break;
                    }
                    jj +=1;
                    if (jj >= Len) {
                        break;
                    }
                }
                if (k.V_r3[aa] > over) {
                    ts.V[aa].push_back((kk-2)*size+jj-k.V_r3[aa]-1);
                    an.V[aa].push_back("over voltage");
                    te.V[aa].push_back((kk-2)*size+jj);
                    report.V[aa] += 1; 
                } else {
                    if(jj-k.V_r3[aa]+k.tmp_V_r3[aa] == 0 && last_sample_V[aa][2] == 1){ //if a swell starts from beginning of a window and we had also a swell at the end of the last window, they are same event.
                        ts.V[aa].pop_back();                                                                      
                        an.V[aa].pop_back();
                        te.V[aa].pop_back();
                    }
                    ts.V[aa].push_back((kk-2)*size+jj-k.V_r3[aa]-1);
                    an.V[aa].push_back("voltage swell");
                    te.V[aa].push_back((kk-2)*size+jj);
                    report.V[aa] += 1; 
                }
                if (jj == Len && k.V_r3[aa] > 0 &&  k.V_r3[aa] <= over){// if the last event in the window was swell, flag the last sample. 
                    last_sample_V[aa][2] = 1;
                    last_sample_V[aa][0] = 0;
                    last_sample_V[aa][1] = 0;
                    k.tmp_V_r3[aa] = k.V_r3[aa];
                } else
                    k.V_r3[aa] = 0;
            }
            else { 
                jj++;
                if (jj >= Len){
                    break;
                }
            }
        }
        if (input[aa][size-1]>0.9 && input[aa][size-1]<1.1){
            last_sample_V[aa][0]=0;
            last_sample_V[aa][1]=0;
            last_sample_V[aa][2]=0;
        }    
    }
    if (report.V[0] > 0 || report.V[1] > 0 || report.V[2] > 0 || last_window_rep.V[0] == 1 || last_window_rep.V[1] == 1 || last_window_rep.V[2] == 1) 
        printReports (v_mag,phasesofintererst);
    for (int ii=0; ii < phasesofintererst; ii++){
        if ((last_sample_V[ii][0]==1 && k.tmp_V_r1[ii] > max_wdw_sag) || last_sample_V[ii][1]==1  || last_sample_V[ii][2]==1)
            last_window_rep.V[ii] = 1;
        else
            last_window_rep.V[ii] = 0;
    }
    for (int ii=0; ii < 3; ii++){
        last_window_val.V[ii]=input[ii][Len-1];
    }
}

void upmuanalyticsASU::printReports(std::vector<double> input[3], int phasesofintererst){// at the end of analysis for each window, this function is called to report the start time, and time and anomaly label of the events found in that window.
    std::ofstream my_log ("full_report.txt", std::ios::app); //report file
    my_log << "-------------Voltage Magnitude Report------------- \n";
    my_log << "Start Time of the Event \n";
    std::vector<int> starttime [3];
    std::vector<int> endtime [3];
    std::vector<std::string> description [3];
    for (int ii=0; ii < phasesofintererst; ii++){
        my_log << phase[ii] << ':'; 
        if ((last_sample_V[ii][0]==1 && k.tmp_V_r1[ii] > max_wdw_sag) || last_sample_V[ii][1]==1  || last_sample_V[ii][2]==1){// if this current window ends with an event that is labeled (e.g., if a sag is less than max_wdw, it is not labeled so we should not care))
            if ( last_window_rep.V[ii] == 1 && ((last_window_val.V[ii]< 0.1 && input[ii][0]>=0.1) || (last_window_val.V[ii]>=0.1 && last_window_val.V[ii]<=0.9 && input[ii][0]<0.1) 
                    || (last_window_val.V[ii]>=0.1 && last_window_val.V[ii]<=0.9 && input[ii][0]>0.9) || (last_window_val.V[ii]>=1.1 && input[ii][0]<1.1) ) ){// if there was a labeled event in the last window which has not continued in the current window, that one should also be included but the last event of the current window should not be reported
                for (int yy=0; yy<report.V[ii]; yy++){ 
                    my_log << ts.V[ii].at(ts.V[ii].size()-report.V[ii]+yy-1) << '\t'; 
                    starttime[ii].push_back(ts.V[ii].at(ts.V[ii].size()-report.V[ii]+yy-1) );
                }
            } else { // if there was no labeled event in the last window or if there was and it has continued in the current window, it means that it has already been taken care of so I do not need to include any event from the last window but I still should not report the last event in the current window.
               for (int yy=0; yy<report.V[ii]-1; yy++){ 
                    my_log << ts.V[ii].at(ts.V[ii].size()-report.V[ii]+yy) << '\t'; 
                    starttime[ii].push_back(ts.V[ii].at(ts.V[ii].size()-report.V[ii]+yy));
                }  
            }
        } else { // if there is no labeled event in the last sample of the current window, 
            if ( last_window_rep.V[ii] == 1 && ((last_window_val.V[ii]< 0.1 && input[ii][0]>=0.1) || (last_window_val.V[ii]>=0.1 && last_window_val.V[ii]<=0.9 && input[ii][0]<0.1) 
                    || (last_window_val.V[ii]>=0.1 && last_window_val.V[ii]<=0.9 && input[ii][0]>0.9) || (last_window_val.V[ii]>=1.1 && input[ii][0]<1.1) ) ){// if there was an event that has not continued 
                for (int yy=0; yy<report.V[ii]+1; yy++){ 
                    my_log << ts.V[ii].at(ts.V[ii].size()-report.V[ii]+yy-1) << '\t'; 
                    starttime[ii].push_back(ts.V[ii].at(ts.V[ii].size()-report.V[ii]+yy-1));
                }                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               
            } else {
               for (int yy=0; yy<report.V[ii]; yy++){ 
                    my_log << ts.V[ii].at(ts.V[ii].size()-report.V[ii]+yy) << '\t'; 
                    starttime[ii].push_back(ts.V[ii].at(ts.V[ii].size()-report.V[ii]+yy));
                }
            }         
        }
        my_log << "\n";
    }
    my_log << "End Time of the Event \n";
    for (int ii=0; ii < phasesofintererst; ii++){
        my_log << phase[ii] << ':'; 
        if ((last_sample_V[ii][0]==1 && k.tmp_V_r1[ii] > max_wdw_sag) || last_sample_V[ii][1]==1  || last_sample_V[ii][2]==1){
            if ( last_window_rep.V[ii] == 1 && ((last_window_val.V[ii]< 0.1 && input[ii][0]>=0.1) || (last_window_val.V[ii]>=0.1 && last_window_val.V[ii]<=0.9 && input[ii][0]<0.1) 
                    || (last_window_val.V[ii]>=0.1 && last_window_val.V[ii]<=0.9 && input[ii][0]>0.9) || (last_window_val.V[ii]>=1.1 && input[ii][0]<1.1) ) ){
                for (int yy=0; yy<report.V[ii]; yy++){ 
                    my_log << te.V[ii].at(te.V[ii].size()-report.V[ii]+yy-1) << '\t'; 
                    endtime[ii].push_back(te.V[ii].at(te.V[ii].size()-report.V[ii]+yy-1));
                }
            } else {
               for (int yy=0; yy<report.V[ii]-1; yy++){ 
                    my_log << te.V[ii].at(te.V[ii].size()-report.V[ii]+yy) << '\t'; 
                    endtime[ii].push_back(te.V[ii].at(te.V[ii].size()-report.V[ii]+yy) );
                }  
            }
        } else {
            if ( last_window_rep.V[ii] == 1 && ((last_window_val.V[ii]< 0.1 && input[ii][0]>=0.1) || (last_window_val.V[ii]>=0.1 && last_window_val.V[ii]<=0.9 && input[ii][0]<0.1) 
                    || (last_window_val.V[ii]>=0.1 && last_window_val.V[ii]<=0.9 && input[ii][0]>0.9) || (last_window_val.V[ii]>=1.1 && input[ii][0]<1.1) ) ){
                for (int yy=0; yy<report.V[ii]+1; yy++){ 
                    my_log << te.V[ii].at(te.V[ii].size()-report.V[ii]+yy-1) << '\t';
                    endtime[ii].push_back(te.V[ii].at(te.V[ii].size()-report.V[ii]+yy-1));
                    
                }                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               
            } else {
               for (int yy=0; yy<report.V[ii]; yy++){ 
                    my_log << te.V[ii].at(te.V[ii].size()-report.V[ii]+yy) << '\t'; 
                    endtime[ii].push_back(te.V[ii].at(te.V[ii].size()-report.V[ii]+yy));
                }
            }         
        }
        my_log << "\n";
    }
    my_log << "Label of the Event \n";
    for (int ii=0; ii < phasesofintererst; ii++){
        my_log << phase[ii] << ':'; 
        if ((last_sample_V[ii][0]==1 && k.tmp_V_r1[ii] > max_wdw_sag) || last_sample_V[ii][1]==1  || last_sample_V[ii][2]==1){
            if ( last_window_rep.V[ii] == 1 && ((last_window_val.V[ii]< 0.1 && input[ii][0]>=0.1) || (last_window_val.V[ii]>=0.1 && last_window_val.V[ii]<=0.9 && input[ii][0]<0.1) 
                    || (last_window_val.V[ii]>=0.1 && last_window_val.V[ii]<=0.9 && input[ii][0]>0.9) || (last_window_val.V[ii]>=1.1 && input[ii][0]<1.1) ) ){
                for (int yy=0; yy<report.V[ii]; yy++){ 
                    my_log << an.V[ii].at(an.V[ii].size()-report.V[ii]+yy-1) << '\t'; 
                    description[ii].push_back(an.V[ii].at(an.V[ii].size()-report.V[ii]+yy-1));
                }
            } else {
               for (int yy=0; yy<report.V[ii]-1; yy++){ 
                    my_log << an.V[ii].at(an.V[ii].size()-report.V[ii]+yy) << '\t'; 
                    description[ii].push_back(an.V[ii].at(an.V[ii].size()-report.V[ii]+yy));
                }  
            }
        } else {
            if ( last_window_rep.V[ii] == 1 && ((last_window_val.V[ii]< 0.1 && input[ii][0]>=0.1) || (last_window_val.V[ii]>=0.1 && last_window_val.V[ii]<=0.9 && input[ii][0]<0.1) 
                    || (last_window_val.V[ii]>=0.1 && last_window_val.V[ii]<=0.9 && input[ii][0]>0.9) || (last_window_val.V[ii]>=1.1 && input[ii][0]<1.1) ) ){
                for (int yy=0; yy<report.V[ii]+1; yy++){ 
                    my_log << an.V[ii].at(an.V[ii].size()-report.V[ii]+yy-1) << '\t'; 
                    description[ii].push_back(an.V[ii].at(an.V[ii].size()-report.V[ii]+yy-1));
                }                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               
            } else {
               for (int yy=0; yy<report.V[ii]; yy++){ 
                    my_log << an.V[ii].at(an.V[ii].size()-report.V[ii]+yy) << '\t'; 
                    description[ii].push_back( an.V[ii].at(an.V[ii].size()-report.V[ii]+yy));
                }
            }         
        }
        my_log << "\n";
    }
    int severity=10;
    my_log.close();
    for (int j=0; j<phasesofintererst;j++){
        for (int i=0;i< starttime[j].size();i++){
            reportp1.push_back(makereport(j, starttime[j].at(i), endtime[j].at(i),  severity,description[j].at(i)));
            
        }
    
    }
    
    
    
} 
            
        
        
        
        
}
