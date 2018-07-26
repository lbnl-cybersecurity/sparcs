/*
 */

/* 
 * File:   serviceInstanceTemplateApp2.h
 * Author: mcp
 *
 * Created on January 24, 2016, 5:01 PM
 */

#ifndef SERVICEDATAARCHIVERRABBITMQ_H
#define SERVICEDATAARCHIVERRABBITMQ_H

namespace serviceCommon {
    /* shared message logger objects */
    extern log4cpp::Appender * logger_appender;
    extern log4cpp::Category& logger_postgres;
    extern log4cpp::Category& logger_cassandra;
    extern log4cpp::Category& logger_curl_ftp;
    extern log4cpp::Category& logger_rabbitmq;
    /* postgres and cassandra operations logger objects */
    serviceOperationsLoggerPostgres * postgresOperationsLogger = NULL;
    serviceOperationsLoggerCassandra * cassandraOperationsLogger = NULL;
    /* uPMU id */
    extern std::string upmuSerialNumber;
    /* upmuGateway genration ID */
    unsigned int sessionID;
    /* counter for allocated uPMU data file buffers */
    extern std::atomic<unsigned int> sharedInMemDataBufferCnt;
    extern std::atomic<unsigned int> sharedTotalDataBufferCnt;
    ///extern serviceInstanceList * services;
    /* uPMU ftp server config info */
    extern bool ftpConnectionValid;
    extern std::string ftpProbeCmd;
    extern std::string ftpAccessString;
    extern std::string ftpUsername;
    extern std::string ftpPasswd;
    extern std::string ftpIpAddress;
    extern int ftpPort;
    extern bool ftpCommEnabled;
    /* cassandra connection context */
    extern CassFuture * cassConnect_future;
    extern CassCluster * cassCluster;
    extern CassSession * cassSession;
    extern CassFuture * cassClose_future;
    extern CassStatement * cassStatementData;
    extern const CassPrepared * cassPreparedData;
    extern const CassPrepared * cassPreparedMetadata;
    extern const CassPrepared * cassPreparedConfiguration;
    extern const CassPrepared * cassPreparedAnnotations;
    extern const CassPrepared * cassPreparedOperations;
    extern std::string cassKeyspace;
    extern std::string cassDataTable;
    extern std::string cassMetadataTable;
    extern std::string cassConfigurationTable;
    extern std::string cassAnnotationsTable;
    extern std::string cassOperationsTable;
    extern std::string cassIpAddress;
    extern int cassPort;
    extern bool cassArchivingEnabled;
    extern bool cassConnected;
}

namespace serviceDataArchiverRabbitMQ {
using namespace pqxx;
using namespace serviceCommon;
using namespace AmqpClient;
    
#define CMD_QUEUE_LEN 32
#define DATA_QUEUE_LEN 16
#define RABBITMQ_MAX_FRAME 131072

#define rabbit_facility "LBL"
#define rabbit_system "rainer_todo" //This will be changed to device id TODO
#define rabbit_host "localhost"
#define rabbit_meter "upmu_location"
#define rabbit_poller "rainer_bbb"

typedef struct bufferProcessingRecord_t {
    int numRecords;
    unsigned int nextRecord;   
    int numInsertsInAccum;
    float accumInsertTime;
} BUFFER_PROCESSING_RECORD;

class serviceDataArchiverRabbitMQ : public serviceInstanceApi {
private:
    std::string serviceName;
    boost::thread* m_thread; // This thread runs this object
    /* Variable that indicates to stop and the mutex to synchronise access */
    bool m_mustStop;
    boost::mutex m_mustStopMutex;
    std::string serviceState;
    log4cpp::Category& logger_serviceDataArchiverRabbitMQ = 
            log4cpp::Category::getInstance(std::string("serviceDataArchiverRabbitMQ"));
    /* serviceDataArchiver methods */
    bool initService();
    void createServiceStartupKeyValueList(upmuStructKeyValueList * kvList);
    void processCmd(SERVICE_COMMAND *);
    int preProcessData(BUFFER_PROCESSING_RECORD *,
        std::shared_ptr<serviceDataBuffer>&);
    bool processData(BUFFER_PROCESSING_RECORD *, 
        std::shared_ptr<serviceDataBuffer>&);
    BUFFER_PROCESSING_RECORD bpr;
    boost::mutex m_dataQueueMutex;
    std::queue<shared_ptr<serviceDataBuffer>> dataQueue;
    shared_ptr<serviceDataBuffer> dataPtr;
    std::string rabbitmqRoutekey;
    std::string rabbitmqExchange;
    std::string rabbitmqVhost;
    std::string rabbitmqUser;
    std::string rabbitmqPasswd;
    std::string rabbitmqIpAddress; 
    std::string rabbitmqlocation;
    int rabbitmqPort;
    bool rabbitmqArchivingEnabled;
    bool rabbitmqConnected;
    Channel::ptr_t rabbitmqWriteChannel;
    Channel::ptr_t channel2;
    BasicMessage::ptr_t rabbitmqMsg;
    amqp_basic_properties_t rabbitmqProp; 
    amqp_bytes_t_ rabbitmqPkt;
    int rabbitmqDataArchive(const char* exchange,const  char* routekey);
    int cassandraCommonArchive(const CassPrepared * prepared,
        uint32_t statsIndex,
        uint64_t timestampMsec, uint64_t day,
        std::string device, uint32_t domainType, uint32_t msgType,
        void * buffer, uint32_t byteLen);
    unsigned char getlocalmac();
    std::string getlocalip();
    std::string localip;
    unsigned char localmac;
    int rabbitreconnectcounter=0;
    float unwr_tmp_new[6] ={};
    float unwr_base[6]={};
    float unwr_tmp_old[6]={180,180,180,180,180,180};
    float unwr_tmp_very_old[6]={180,180,180,180,180,180};
    float unwr_tmp_very_very_old[6]={180,180,180,180,180,180};
    float unwrderivative[6][3]={};
    unsigned int old_hour=0;
    

    
public:
    serviceDataArchiverRabbitMQ();
    ~serviceDataArchiverRabbitMQ();
    
    void start();
    void stop();
     
    dataBufferOffer_t dataBufferOffer(const std::shared_ptr<serviceDataBuffer>&);
    void operator () ();
    void setServiceName(std::string s) { this->serviceName = s;};
    std::string getServiceName() { return this->serviceName;};
    
    dbConnection * dbConn;
};

#define SCAN_MODE_CATCH_UP_DELAY_SEC 1
#define SCAN_MODE_NORMAL_DELAY_SEC 2
}

#endif /* SERVICEDATAARCHIVERRABBITMQ_H */

