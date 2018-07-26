/*
 */

/* 
 * File:   serviceInstanceTemplateApp2.h
 * Author: mcp
 *
 * Created on January 24, 2016, 5:01 PM
 */

#ifndef SERVICEDATADISCOVERY_H
#define SERVICEDATADISCOVERY_H

namespace serviceCommon {
    /* shared message logger objects */
    extern log4cpp::Appender * logger_appender;
    extern log4cpp::Category& logger_postgres;
    extern log4cpp::Category& logger_cassandra;
    extern log4cpp::Category& logger_curl_ftp;
    extern log4cpp::Category& logger_rabbitmq;
    /* postgres and cassandra operations logger objects */
    extern serviceOperationsLoggerPostgres * postgresOperationsLogger;
    extern serviceOperationsLoggerCassandra * cassandraOperationsLogger;   
    /* uPMU id */
    extern std::string upmuSerialNumber;
    /* upmuGateway genration ID */
    extern unsigned int sessionID;
    /* counter for allocated uPMU data file buffers */
    extern std::atomic<unsigned int> sharedInMemDataBufferCnt;
    extern std::atomic<unsigned int> sharedTotalDataBufferCnt;
    /* list of configured services, status and thread pointer */
    extern serviceInstanceList * services;
    /* uPMU ftp server config info */
    extern bool ftpConnectionValid;
    extern std::string ftpProbeCmd;
    extern std::string ftpAccessString;
    extern std::string ftpUsername;
    extern std::string ftpPasswd;
    extern std::string ftpIpAddress;
    extern int ftpPort;
    extern bool ftpCommEnabled;
    extern CURL * curlDirServices;
    extern CURL * curlDataTransServices;
    extern CURL * curlFileInfoServices;
    extern CURL * curlHttpServices;
    extern CURL * curlHttpServices;
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

namespace serviceDataDiscovery {
using namespace pqxx;
using namespace serviceCommon;
    
#define DEFAULT_STARTING_YEAR 2016
#define DEFAULT_STARTING_MONTH 5
#define DEFAULT_STARTING_DAY 1
#define DEFAULT_STARTING_HOUR 0

#define STATE_NORMAL "normal"
#define STATE_CATCH_UP "catch_up"

#define CMD_QUEUE_LEN 32
#define DATA_QUEUE_LEN 16
#define MAX_OUTSTANDING_MEM_BUF 5
#define SCAN_MODE_CATCH_UP_DELAY_SEC 1
#define SCAN_MODE_NORMAL_DELAY_SEC 15
#define CATCH_UP_VS_NORMAL_THRESH_SEC 200

class serviceDataDiscovery : public serviceInstanceApi {
private:
    std::string serviceName;
    boost::thread* m_thread; // The thread runs this object
    int m_msg_num; // The current frame number
    // Variable that indicates to stop and the mutex to
    // synchronise "must stop" on (mutex explained later)
    bool m_mustStop;
    boost::mutex m_mustStopMutex; 
    std::string serviceState;
    /* create log4cpp related objects for this service. */
    log4cpp::Category& logger_serviceDataDiscovery = 
            log4cpp::Category::getInstance(std::string("serviceDataDiscovery"));
    void processCmd(SERVICE_COMMAND *);
    void privateMethod();
    bool initService();
    void createServiceStartupKeyValueList(upmuStructKeyValueList * kvList);
    void processUPMUdata(std::string ipAddress);
    
    void swapBytesUint(unsigned int *);
    int cassandraCommonArchive(const CassPrepared * prepared,
        uint32_t statsIndex,
        uint64_t timestampMsec, uint64_t day,
        std::string device, uint32_t domainType, uint32_t msgType,
        void * buffer, uint32_t byteLen);
    DATA_DIR *dataDir;
    bool dataDirValid;
    dbConnection * dbConn; 
    bool dataDirAccessInProgress;
    bool mostRecentDataFileNameValid;
    std::string mostRecentDataFileName;
    time_t dataDirAccessStartTime;
    unsigned int mostRecentDataFileLength;
    std::set<std::string> dataFilesInDir;
    std::set<std::string> dataFilesProcessed;
    enum DATA_FILE_SCAN_MODE {catch_up, normal} scanMode;
    
public:
    serviceDataDiscovery();
    ~serviceDataDiscovery();
    void start();
    void stop();
    dataBufferOffer_t dataBufferOffer(const std::shared_ptr<serviceDataBuffer>&);
    void operator () ();
    void setServiceName(std::string s) { serviceName = s;};
    std::string getServiceName() { return serviceName;};
    upmuFtpAccess * upmuDaq;
    std::set<std::string> * dataFiles;

};

}

#endif /* SERVICEDATADISCOVERY_H */

