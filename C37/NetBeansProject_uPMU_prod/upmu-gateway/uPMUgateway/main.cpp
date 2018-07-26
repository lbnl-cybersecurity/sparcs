/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   main.cpp
 * Author: mcp
 *
 * Created on January 10, 2016, 8:55 AM
 */



#include <cstdlib>
#include <iostream>
#include <fstream>
#include <mutex>
#include <pqxx/pqxx>
#include <curl/curl.h>
#include <string>
#include <unordered_map>
#include <chrono>
#include <thread>
#include <boost/dll/import.hpp>
#include <boost/thread/condition.hpp>
#include <boost/dll/import.hpp>
#include <boost/any.hpp>
#include <log4cpp/Appender.hh>
#include <log4cpp/Category.hh>
#include <log4cpp/FileAppender.hh>
#include <log4cpp/OstreamAppender.hh>
#include <log4cpp/Layout.hh>
#include <log4cpp/BasicLayout.hh>
#include <log4cpp/Priority.hh>

#include "dbConnection.h"
#include "cassandra.h"
#include "domainAndMessageTypes.h"
#include "serviceOperationsLoggerPostgres.h"
#include "serviceOperationsLoggerCassandra.h"
#include "serviceDataBuffer.h"
#include "serviceCommandQueueTemplate.h"
#include "serviceInstanceApi.h"
#include "serviceInstance.h"
#include "serviceInstanceList.h"
#include "upmuStructKeyValueListProtobuf.pb.h"

using namespace std;
using namespace pqxx;
using namespace serviceCommon;
using namespace boost;
using namespace log4cpp;

typedef struct Credentials_ {
  const char* username;
  const char* password;
} Credentials;


namespace serviceCommon {
    /* shared message logger objects */
    log4cpp::Appender * logger_appender;
    log4cpp::Category& logger_postgres = log4cpp::Category::getInstance(std::string("postgres"));
    log4cpp::Category& logger_cassandra = log4cpp::Category::getInstance(std::string("cassandra"));
    log4cpp::Category& logger_curl_ftp = log4cpp::Category::getInstance(std::string("curl_ftp"));
    log4cpp::Category& logger_uPMUgateway = log4cpp::Category::getInstance(std::string("uPMUgateway"));
    log4cpp::Category& logger_rabbitmq = log4cpp::Category::getInstance(std::string("rabbitMQ"));
    /* postgres and cassandra operations logger objects */
    serviceOperationsLoggerPostgres * postgresOperationsLogger = NULL;
    serviceOperationsLoggerCassandra * cassandraOperationsLogger = NULL;
    /* uPMU id */
    std::string upmuSerialNumber;
    /* upmuGateway genration ID */
    unsigned int sessionID;
    /* counter for  allocated uPMU data file buffers */
    std::atomic<unsigned int> sharedInMemDataBufferCnt;
    std::atomic<unsigned int> sharedTotalDataBufferCnt;
    /* uPMU ftp server config info */
    bool ftpConnectionValid;
    std::string ftpProbeCmd;
    std::string ftpAccessString;
    std::string ftpUsername;
    std::string ftpPasswd;
    std::string ftpIpAddress;
    int ftpPort;
    bool ftpCommEnabled;
    CURL * curlDirServices;
    CURL * curlDataTransServices;
    CURL * curlFileInfoServices;
    CURL * curlHttpServices;
    /* list of configured services, status and thread pointer */
    serviceInstanceList * services;
    /* cassandra connection context */
    CassFuture * cassConnect_future;
    CassCluster * cassCluster;
    CassSession * cassSession;
    CassFuture * cassClose_future;
    const CassPrepared * cassPreparedData;
    const CassPrepared * cassPreparedMetadata;
    const CassPrepared * cassPreparedConfiguration;
    const CassPrepared * cassPreparedAnnotations;
    const CassPrepared * cassPreparedOperations;
    std::string cassKeyspace;
    std::string cassDataTable;
    std::string cassMetadataTable;
    std::string cassConfigurationTable;
    std::string cassAnnotationsTable;
    std::string cassOperationsTable;
    std::string cassIpAddress;
    int cassPort;
    bool cassArchivingEnabled;
    bool cassConnected;
}

#define UPMU_PRESENT

#define SUCCESS 0
#define FAIL -1
/* major and minor version info for this service.  Note:
 this is handled differently in plugins....see serviceSpecificStats .cpp and .h */
#define UPMU_GATEWAY_MAJOR_VERSION "0.9"
#define UPMU_GATEWAY_MINOR_VERSION "33"
#define LBL_UPMU_GATEWAY_MAJOR_VER "Data_Discovery_Major_Version"
#define LBL_UPMU_GATEWAY_MINOR_VER "Data_Discovery_Minor_Version"

void on_auth_initial(CassAuthenticator* auth,
                       void* data) {
  /*
   * This callback is used to initiate a request to begin an authentication
   * exchange. Required resources can be acquired and initialized here.
   *
   * Resources required for this specific exchange can be stored in the
   * auth->data field and will be available in the subsequent challenge
   * and success phases of the exchange. The cleanup callback should be used to
   * free these resources.
   */

  /*
   * The data parameter contains the credentials passed in when the
   * authentication callbacks were set and is available to all
   * authentication exchanges.
   */
    
  const Credentials* credentials = (const Credentials *)data;

 // size_t username_size = strlen(credentials->username);
  //size_t password_size = strlen(credentials->password);
  std::cout<< "Cassandra auth callback" << std::endl;
  size_t username_size = strlen("cassandrauser");
  size_t password_size = strlen("cassandrapassword");

  size_t size = username_size + password_size + 2;

  char* response = cass_authenticator_response(auth, size);

  /* Credentials are prefixed with '\0' */
  response[0] = '\0';
  memcpy(response + 1, "cassandrauser", username_size);

  response[username_size + 1] = '\0';
  memcpy(response + username_size + 2, "cassandrapassword", password_size);
}

void on_auth_challenge(CassAuthenticator* auth,
                       void* data,
                       const char* token,
                       size_t token_size) {
  std::cerr << "Cassandra Server requested auth. Responding now..."<< std::endl;
  const Credentials* credentials = (const Credentials *)data;
  size_t username_size = strlen("cassandrauser");
  size_t password_size = strlen("cassandrapassword");

  size_t size = username_size + password_size + 2;

  char* response = cass_authenticator_response(auth, size);

  /* Credentials are prefixed with '\0' */
  response[0] = '\0';
  memcpy(response + 1, "cassandrauser", username_size);

  response[username_size + 1] = '\0';
  memcpy(response + username_size + 2, "cassandrapassword", password_size);
}


void on_auth_success(CassAuthenticator* auth,
                     void* data,
                     const char* token,
                     size_t token_size ) {
  /*
   * Not used for plain text authentication, but this is to be used
   * for handling the success phase of an exchange.
   */
    std::cout<< "Successful authenticated against Cassandra" << std::endl;
}

void on_auth_cleanup(CassAuthenticator* auth, void* data) {
  /*
   * No resources cleanup is necessary for plain text authentication, but
   * this is used to cleanup resources acquired during the authentication
   * exchange.
   */
}


std::string getUPMUSerialNumber();
unsigned int getLatestSessionID(dbConnection * dbConn, std::string serialNum);
void createServiceStartupKeyValueList(upmuStructKeyValueList * kvList);
int cassandraInit(dbConnection * dbConn);
int cassandraConnect();
int cassandraInit(dbConnection * dbConn);
int cassandraConnect();
void on_auth_initial(CassAuthenticator* auth,
                       void* data);
void on_auth_challenge(CassAuthenticator* auth,
                       void* data,
                       const char* token,
                       size_t token_size);








/* local storage */
/* Note:  All service plugins get their "official" name from the database.
 uPMUgateway is the only exception and is thus has its name defined here. */
std::string serviceName("uPMUgateway");
/* similarly for serviceState */
std::string serviceState("running");

/* write to memory struct and callback */
struct MemoryStruct {
    char * memory;
    size_t size;
};

/* this is a callback used by curl.  It has to go somewhere, so we'll sead with it. */
size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userrp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *) userrp;
    mem->memory = (char *)std::realloc(mem->memory, mem->size + realsize + 1);
    if(mem->memory == NULL) {
        printf("not enough memory\n");
        return 0;
    }
    std::memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;
    return realsize;
}

int main(int argc, char* argv[])
{
    /* first, set up message logger objects */
    logger_appender = new log4cpp::FileAppender("default", 
        "/var/log/upmugateway/upmugateway.log", false, (mode_t) 00755);
    logger_appender->setLayout(new log4cpp::BasicLayout());
    
    /* init category objects */
    logger_postgres.addAppender(logger_appender);
    logger_cassandra.addAppender(logger_appender);
    logger_curl_ftp.addAppender(logger_appender);
    logger_uPMUgateway.addAppender(logger_appender);
    logger_rabbitmq.addAppender(logger_appender);
    
    logger_uPMUgateway.info("uPMUgateway starting.");
    logger_uPMUgateway.info("All loggers initialized.");
    
    /* get a db connection for local database */
    dbConnection * dbConn = dbConnection::getInstance();
    if(dbConn->conn == NULL) {
        logger_postgres.error("uPMUgateway: Cannot access local database or uPMU_comm records are corrupted.");
        return (EXIT_FAILURE);
    }
    
    /* create object that inserts operations records into postgres operations
     table.  THis object is shared with plugins as well.  Set to NULL if
     object cannot be created */
    try {
        postgresOperationsLogger = new serviceOperationsLoggerPostgres(dbConn);
    }
    catch (std::exception e) {
        postgresOperationsLogger = NULL;
        logger_postgres.warn("uPMUgateway: Operations logger could not be created");
    }
    
    /* init cassandra connection */
    int sts = cassandraInit(dbConn);
    if(sts < 0) {
        logger_uPMUgateway.warn("Cassandra connection failed, proceeding anyway.");          
    }
    else {
        /* create cassandra operations logger object (see above) */
        cassandraOperationsLogger = new serviceOperationsLoggerCassandra();   
    }
    
    /* create service instance list */
    services = serviceInstanceList::getInstance();
    
    /* probe for ftp ip connection */
    ftpConnectionValid = false;
    
    /* lock DB access */
    dbConn->acquireDBaccess();   
    nontransaction nt0(*(dbConn->conn));
    result r = nt0.exec("SELECT * FROM ftp_comm WHERE state = 'current'");
    if(r.size() == 1) {
        pqxx::result::field field = r[0]["\"probe_cmd\""];
        std::string probeCmd = field.c_str();
        /* test */
        //probeCmd = "/2017/";
        field = r[0]["\"username\""];
        ftpUsername = field.c_str();
        /* test */
        //ftpUsername = "ftp_user_1";
        field = r[0]["\"passwd\""];
        ftpPasswd = field.c_str();
        /* test */
        //ftpPasswd = "ForPSLLBNL";
        field = r[0]["\"ip_address\""];
        ftpIpAddress = field.c_str();
        /* test */
        //ftpIpAddress = "192.168.1.197";
        field = r[0]["\"port\""];
        ftpPort = field.as<int>();
        field = r[0]["\"comm_enabled\""];
        ftpCommEnabled = field.as<bool>();

        ftpAccessString.assign("ftp://");
        ftpAccessString.append(ftpIpAddress);
        ftpAccessString.append(":");
        std::string p = std::to_string(ftpPort);
        ftpAccessString.append(p);
        ftpProbeCmd.assign(ftpAccessString);
        ftpProbeCmd.append(probeCmd); 
        logger_postgres.info("uPMUgateway: uPMU (IP = " + ftpIpAddress +
                ") access params configured from postgres.");          
        
    }
    else {
       logger_postgres.error("uPMUgateway: Cannot access local database"
            " or uPMU_curl_ftp records corrupted.");
       exit(-1);
    }
    /* close open postgres transaction */
    nt0.commit();
    dbConn->releaseDBaccess();
     
    /* try to connect with uPMU ftp service */
    logger_curl_ftp.info("uPMUgateway: Attempting to connect to uPMU (IP = " + 
            ftpIpAddress + ").  Additional log messages indicate success.");
    
#ifdef UPMU_PRESENT
    
    while (!ftpConnectionValid) {   
        /* init curl library */
        curlDirServices = NULL;
        curlDataTransServices = NULL;
        curlFileInfoServices = NULL;
        curlHttpServices = NULL;
        curl_global_init(CURL_GLOBAL_ALL);

        CURL * curl;
        CURLcode res;
        curl = curl_easy_init();
        if(curl) {
            curl_easy_setopt(curl, CURLOPT_USERNAME, ftpUsername.c_str());
            curl_easy_setopt(curl, CURLOPT_PASSWORD, ftpPasswd.c_str());
            curl_easy_setopt(curl, CURLOPT_URL, ftpProbeCmd.c_str());
            curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
            curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);
            res = curl_easy_perform(curl);

            if(res == CURLE_OK) {
                /* get uPMU serial number */
                upmuSerialNumber = getUPMUSerialNumber();

            }
            if(upmuSerialNumber.compare("") == 0) {
                /* if we don't have a valid serial number, we cannot log data. */
                ftpConnectionValid = false;
            }
            else {
                ftpConnectionValid = true;
            }
            curl_easy_cleanup(curl);    
        }
    
        if(!ftpConnectionValid) { /* fix ! */
            /* either bad db configuration or could not reach uPMU */
            /* sleep a bit and retry again later. Cannot really do anything until
             we can talk to the uPMU, so we wait. */
            std::cout << "Errors in estabishing uPMU ftp access.  Sleeping and will retry in 30 sec." << std::endl;
            ftpConnectionValid = true;; //test
            /* altered for testing McP */
            //std::this_thread::sleep_for(std::chrono::seconds(30));
        }  
    } 
    
#else
    /* this allows easy recompilation for testing in environment without a upmu. */
    upmuSerialNumber = "No_uPMU_Present";
    ftpConnectionValid = false;
    
#endif
    
    logger_curl_ftp.info("uPMUgateway: uPMU (IP = " + ftpIpAddress +
                ") connect successful. Connected device serial number is " +
                upmuSerialNumber + ".");          
     
    /* Get our session id for this instance. */
    sessionID = getLatestSessionID(dbConn, upmuSerialNumber);
    
    /* declare uPMUgateway started at this point. Note: startup message always 
     contains sessionID and major/minor version info.  Also, these params are ad hoc
     in the gateway; but are part of standard plugin statistices package in plugins. */
    postgresOperationsLogger->createStringOperationsRecord(dbConn, serviceState, upmuSerialNumber,
        sessionID, serviceName, "Service_Startup_Msg", "Loading and starting plugins");
    postgresOperationsLogger->createStringOperationsRecord(dbConn, serviceState, upmuSerialNumber,
        sessionID, serviceName, LBL_UPMU_GATEWAY_MAJOR_VER, UPMU_GATEWAY_MAJOR_VERSION);
    postgresOperationsLogger->createStringOperationsRecord(dbConn, serviceState, upmuSerialNumber,
        sessionID, serviceName, LBL_UPMU_GATEWAY_MINOR_VER, UPMU_GATEWAY_MINOR_VERSION);
   
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
              std::cout<<"failure to free memory main.cpp";  
            }
        } 
        else {
            logger_uPMUgateway.error("Cannot form serialized protocol buffer, "
                "insufficient memory."); 
        }
    }

    /* Now we get list of services to start, load their libraries and
     start each in their own thread. */
    /* lock DB access */
    dbConn->acquireDBaccess();   
    nontransaction nt1(*(dbConn->conn));
    /* get list of services from database */
    r = nt1.exec("SELECT * FROM service WHERE service_active is true");
    /* iterate through list */
    for (pqxx::result::const_iterator row = r.begin(); row != r.end(); row++) {        
        std::string serviceName;
        std::string serviceImplementation;
        std::string serviceExecMode;
        std::string serviceLibPath;
        bool serviceAcceptsData;
        int serviceQueueDepth;
        int serviceMask;
        
        try {
            /* get important fields into local variables */
            pqxx::result::field field = row->at("service_name");
            serviceName = field.c_str();

            field = row->at("service_implementation");
            serviceImplementation = field.c_str();
            
            field = row->at("service_exec_mode");
            serviceExecMode = field.c_str();

            /* pick the appropriate path to find loadable code. */
            if(serviceExecMode.compare("debug") == 0) {
                field = row->at("service_debug_lib_path");
            }
            else {
                field = row->at("service_lib_path");
            }
            serviceLibPath = field.c_str();

            field = row->at("service_accepts_data");
            serviceAcceptsData = field.as<bool>();

            field = row->at("service_queue_depth");
            serviceQueueDepth = field.as<int>();

            field = row->at("service_mask");
            serviceMask = field.as<int>();
        
            serviceInstance * si = new serviceInstance(serviceName,
                    serviceImplementation, serviceLibPath, serviceAcceptsData,
                    serviceMask, serviceQueueDepth);

            services->createServiceInstance(si);
        }
        catch (const std::exception &e) {
            /* log problem */
            logger_uPMUgateway.error("Exception while attempting to start "
                + serviceName + " thread.  Exception was " + e.what());
        }
    }
   /* close open postgres transaction */
    nt1.commit();
    dbConn->releaseDBaccess();
    
    
    /* start threads; but wait to start serviceDataDiscovery last to
     avoid any high level synchronization issues */
    for (auto si: services->services) {
        if(si.first.compare("serviceDataDiscovery") != 0) {
            si.second->serviceThread->setServiceName(si.second->serviceName);
            si.second->serviceThread->start();
        };
    }
    /* sleep for a bit */
    std::this_thread::sleep_for(std::chrono::seconds(10));      
    if(services->services.count("serviceDataDiscovery") > 0) {
        serviceInstance * si = services->services.at("serviceDataDiscovery");
        si->serviceThread->setServiceName(si->serviceName);
        si->serviceThread->start();
    }
    
    for(;;) {
        /* this loop continues process execution.  In the future, we will spawn
         a web server here and deal with data requests. */
        std::this_thread::sleep_for(std::chrono::seconds(30));
    }
}

std::string getUPMUSerialNumber() {
    /* curl specific structs */
    //CURL * curlHttpServices;
    CURLcode res;
    struct MemoryStruct chunk;
    chunk.memory = (char *)std::malloc(1000);
    chunk.size = 0;
    
    /* uPMU serial number...."" indicates could not obtain it for some reason. */
    std::string serialNumber("");
    
    if(curlHttpServices == NULL) {
        curlHttpServices = curl_easy_init();
    }
            
    if(curlHttpServices) {   
        std::string cmd = "http://";
        cmd.append(ftpIpAddress);
        cmd.append("/html_status.cgi/");
        curl_easy_setopt(curlHttpServices, CURLOPT_URL, cmd.c_str());
        curl_easy_setopt(curlHttpServices, CURLOPT_WRITEFUNCTION, 
            WriteMemoryCallback);
        curl_easy_setopt(curlHttpServices, CURLOPT_WRITEDATA, (void *)&chunk);
        //curl_easy_setopt(curlDirServices, CURLOPT_USERAGENT, "libcurl-agent/1.0");
        res = curl_easy_perform(curlHttpServices);
        
        if(res == CURLE_OK) {   
            std::string tag0("PQube 3 Serial Number:");
            std::string tag1("<TD>");
            std::string tag2("</TD>");
            std::string page(chunk.memory);
            std::size_t startPos = page.find(tag0);
            startPos = page.find(tag1, startPos + 1);
            std::size_t endPos = page.find(tag2, startPos + 1);
            serialNumber = page.substr(startPos + 4, endPos - startPos - 4);

            //std::cout << "Serial Number: " << serialNumber << std::endl;
        }    
    }
    /* free up the receive buffer */
    std::free(chunk.memory);
    /* clean up curl structures */
    if(curlHttpServices != NULL) {
        curl_easy_cleanup(curlHttpServices);
    }
    curlHttpServices = NULL;
    return serialNumber;
}

unsigned int getLatestSessionID(dbConnection * dbConn, std::string serialNum) {
    unsigned int id = 0;
    /* lock DB access */
    dbConn->acquireDBaccess();   
    nontransaction nt0(*(dbConn->conn));
    std::string s = "SELECT MAX(session_id) FROM operations WHERE device = '"
            + serialNum + "' AND component = '" + serviceName + "'";
    result r = nt0.exec(s.c_str());
    if(r.size() == 0) {
        logger_postgres.error("uPMUgateway: unexpected operations table error.");         
    }
    else {
        pqxx::result::field field = r[0][0]; 
        if(field.is_null()) {
            id++;
            logger_uPMUgateway.info("No prior sessions in database.  Starting with "
                "sessionID: 1."); 
        }
        else {
            id = field.as<int>();
            /* bump it by one. */
            id++;
            std::string idStr  = std::to_string(id);
            s = "Prior session located in database.  Starting with sessionID: " + idStr + ".";
            logger_uPMUgateway.info(s.c_str());
        }
    }
    /* close open postgres transaction */
    nt0.commit();
    dbConn->releaseDBaccess();
    return id;
     
}    

void createServiceStartupKeyValueList(upmuStructKeyValueList * kvList) {
    kvList->set_name("Service_Startup_Info");
    uint32_t startTime = (uint32_t)time(NULL);
    kvList->set_timestamp(startTime);
    KeyValueList * list = kvList->add_list();
    list->set_category("ServiceStatus");

    /* add a few key values */
    KeyValue * pair = list->add_element();
    pair->set_key("Service_Startup_Action"); 
    pair->set_stringval("Loading and starting plugins");

    pair = list->add_element();
    pair->set_key(UPMU_GATEWAY_MAJOR_VERSION); 
    pair->set_stringval(LBL_UPMU_GATEWAY_MAJOR_VER);

    pair = list->add_element();
    pair->set_key(UPMU_GATEWAY_MINOR_VERSION); 
    pair->set_stringval(LBL_UPMU_GATEWAY_MINOR_VER);
}
        
int cassandraInit(dbConnection * dbConn) {
/* get list of services from database */
       
    try {
        dbConn->acquireDBaccess();
        nontransaction nt(*(dbConn->conn));
        result r = nt.exec("SELECT * FROM cassandra ORDER BY last_modified DESC LIMIT 1");
        /* should be exactly one row */
        if(r.size() != 1) {
            logger_postgres.error("uPMUgateway: Multiple or missing cassandra config "
                    " records in postgres table: cassandra.");       
            return -1;
        }
        /* get important fields into local variables */
        pqxx::result::field field = r[0]["\"keyspace\""];
        cassKeyspace = field.c_str();
        field = r[0]["\"data_table\""];
        cassDataTable = field.c_str();
        field = r[0]["\"metadata_table\""];
        cassMetadataTable = field.c_str();
        field = r[0]["\"configuration_table\""];
        cassConfigurationTable = field.c_str();
        field = r[0]["\"annotations_table\""];
        cassAnnotationsTable = field.c_str();
        field = r[0]["\"operations_table\""];
        cassOperationsTable = field.c_str();
        field = r[0]["\"ip_address\""];
        cassIpAddress = field.c_str();       
        field = r[0]["\"port\""];
        cassPort = field.as<int>();
        field = r[0]["\"archiving_enabled\""];
        cassArchivingEnabled = field.as<bool>();
        
        nt.commit();
        dbConn->releaseDBaccess();
        
        if(cassArchivingEnabled) {
            /* Setup and connect to cluster */
            int sts = cassandraConnect();
            if(sts >= 0) {
                logger_cassandra.info("uPMUgateway: Cassandra pre-defined queries "
                        "created in server.");
                cassConnected = true;
            }
            else {
                logger_cassandra.error("uPMUgateway: Cassandra pre-defined queries "
                        "could not be created in server.  Connection will be "
                        "disabled.");
                cassConnected = false;
            }
        }
        else {
            cassConnected = false;
            logger_cassandra.info("uPMUgateway: Cassandra access disabled in "
                    "postgres table: cassandra.");
        }
        
    }
    catch (const std::exception &e) {
        /* log problem */
        cassConnected = false;
        
    }
    
    if(cassConnected == true) {
        logger_cassandra.info("uPMUgateway: Cassandra (IP = " + cassIpAddress +
                ") connect successful.");          
        return SUCCESS;
    }
    else {
        logger_cassandra.warn("uPMUgateway: Cassandra (IP = " + cassIpAddress +
                ") connect unsuccessful.");
        return FAIL;
        
    }
}
 

  
int cassandraConnect() {
    cassConnect_future = NULL;
    cassCluster = cass_cluster_new();
    cassSession = cass_session_new();

    /* Add contact points */
    cass_cluster_set_contact_points(cassCluster,cassIpAddress.c_str());
    /* Provide the cluster object as configuration to connect the session */
    
//New add for authentication
     CassAuthenticatorCallbacks auth_callbacks = {
    on_auth_initial,
    on_auth_challenge,
    on_auth_success,
    on_auth_cleanup
  };
    Credentials credentials = {
    "cassandrauser",
    "cassandrapassword"
  };
  cass_cluster_set_authenticator_callbacks(cassCluster,&auth_callbacks,NULL, &credentials);  
//
   
    cassConnect_future = cass_session_connect_keyspace(
        cassSession, cassCluster, cassKeyspace.c_str());

    if (cass_future_error_code(cassConnect_future) != CASS_OK) {
        /* server not available. log error*/
        //
        const char* message;
    size_t message_length;
    cass_future_error_message(cassConnect_future, &message, &message_length);
    fprintf(stderr, "Unable to connect: '%.*s'\n", (int)message_length,
                                                        message);
        //
        
        return -1;
    }
    cassClose_future = NULL;
    
    /* Create preformed queries - they are more efficient. */
    /* Data table */
    std::string s = "INSERT INTO " + cassDataTable +
                " (ID, TIMESTAMP_MSEC, DAY, DEVICE, "
                "DOMAIN_TYPE, MSG_TYPE, DATA)  VALUES "
                "(now(), ?, ?, ?, ?, ?, ?)";
    CassFuture * prepare_future = cass_session_prepare(cassSession, s.c_str());
    CassError rc = cass_future_error_code(prepare_future);
    if(rc != CASS_OK) {
        return -1;
    }
    cassPreparedData = cass_future_get_prepared(prepare_future);
    cass_future_free(prepare_future);
    
    /* Metadata table */
    s = "INSERT INTO " + cassMetadataTable +
                " (ID, TIMESTAMP_MSEC, DAY, DEVICE, "
                "DOMAIN_TYPE, MSG_TYPE, DATA)  VALUES "
                "(now(), ?, ?, ?, ?, ?, ?)";
    prepare_future = cass_session_prepare(cassSession, s.c_str());
    rc = cass_future_error_code(prepare_future);
    if(rc != CASS_OK) {
        return -1;
    }
    cassPreparedMetadata = cass_future_get_prepared(prepare_future);
    cass_future_free(prepare_future);
        
    /* Configuration table */
    s = "INSERT INTO " + cassConfigurationTable +
                " (ID, TIMESTAMP_MSEC, DAY, DEVICE, "
                "DOMAIN_TYPE, MSG_TYPE, DATA)  VALUES "
                "(now(), ?, ?, ?, ?, ?, ?)";
    prepare_future = cass_session_prepare(cassSession, s.c_str());
    rc = cass_future_error_code(prepare_future);
    if(rc != CASS_OK) {
        return -1;
    }
    cassPreparedConfiguration = cass_future_get_prepared(prepare_future);
    cass_future_free(prepare_future);
        
    /* Annotations table */
    s = "INSERT INTO " + cassAnnotationsTable +
                " (ID, TIMESTAMP_MSEC, DAY, DEVICE, "
                "DOMAIN_TYPE, MSG_TYPE, DATA)  VALUES "
                "(now(), ?, ?, ?, ?, ?, ?)";
    prepare_future = cass_session_prepare(cassSession, s.c_str());
    rc = cass_future_error_code(prepare_future);
    if(rc != CASS_OK) {
        return -1;
    }
    cassPreparedAnnotations = cass_future_get_prepared(prepare_future);
    cass_future_free(prepare_future);
    
    /* Operations table */
    s = "INSERT INTO " + cassOperationsTable +
                " (ID, TIMESTAMP_MSEC, DAY, DEVICE, "
                "COMPONENT, SESSION_ID, "
                "DOMAIN_TYPE, MSG_TYPE, DATA)  VALUES "
                "(now(), ?, ?, ?, ?, ?, ?, ?, ?)";
    prepare_future = cass_session_prepare(cassSession, s.c_str());
    rc = cass_future_error_code(prepare_future);
    if(rc != CASS_OK) {
        return -1;
    } 
    cassPreparedOperations = cass_future_get_prepared(prepare_future);
    cass_future_free(prepare_future);
    
    /* success, all prepared statementsss accepted */
    return 0;          
}  

