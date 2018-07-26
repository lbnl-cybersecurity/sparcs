/*
 * Stream-Processing Architecture for Real-time Cyber-physical Security (SPARCS)
 */

#include <mutex>
#include <map>
#include <boost/thread/condition.hpp>
#include <boost/any.hpp>

#include "serviceCommandQueueTemplate.h"
#include "serviceInstanceApi.h"
#include "upmuStructKeyValueListProtobuf.pb.h"

namespace serviceCommon {
    /* counter for allocated uPMU data file buffers */
    extern std::atomic<unsigned int> sharedInMemDataBufferCnt;
    extern std::atomic<unsigned int> sharedTotalDataBufferCnt;
}

using namespace boost;
using namespace serviceCommon;

#include "serviceSpecificStats.h"

/* these need to be static */
/* label storage */
static std::string lbl_bufFmtError(LBL_BUF_FMT_ERROR);
static std::string lbl_inMemSharedDataBuffers(LBL_IN_MEM_SHARED_DATA_BUFFERS);
static std::string lbl_totalSharedDataBuffers(LBL_TOTAL_SHARED_DATA_BUFFERS);
static std::string lbl_abandonedBuffers(LBL_ABANDONED_BUFFERS);
static std::string lbl_pbNoSerialMemAvail(LBL_PB_NO_SERIAL_MEM_AVAIL);
static std::string lbl_cassandraMetadataInsertError(LBL_CASSANDRA_METADATA_INSERT_ERR);
static std::string lbl_cassandraAnnotationsInsertError(LBL_CASSANDRA_ANNOTATIONS_INSERT_ERR);
static std::string lbl_cassandraDataInsertError(LBL_CASSANDRA_DATA_INSERT_ERR);
static std::string lbl_insertAbandonedNoCassandraConn(LBL_INSERT_ABANDON_NO_CASSANDRA_CONN);
static std::string lbl_dataDiscoveryMajVer(LBL_DATA_DISCOVERY_MAJ_VER);
static std::string lbl_dataDiscoveryMinVer(LBL_DATA_DISCOVERY_MIN_VER);

/* value storage **/
static unsigned int val_bufFmtError = 0;
static unsigned int val_abandonedBuffers = 0;
static unsigned int val_pbNoSerialMemAvail = 0;
static unsigned int val_cassandraDataInsertError = 0;
static unsigned int val_cassandraMetadataInsertError = 0;
static unsigned int val_cassandraAnnotationsInsertError = 0;
static unsigned int val_insertAbandonedNoCassandraConn = 0;
static std::string val_dataDiscoveryMajVer(DATA_DISCOVERY_MAJOR_VERSION);
static std::string val_dataDiscoveryMinVer(DATA_DISCOVERY_MINOR_VERSION);

void createServiceSpecificStats(serviceInstanceApi * servApi) {
    (servApi->serviceSpecStatsMutex).lock();
    /* first the labels */
    std::map<unsigned int, std::string *> * lblMap = 
        new std::map<unsigned int, std::string *>();
    lblMap->insert(std::make_pair(INDX_UINT_BUF_FMT_ERROR, &lbl_bufFmtError));
    lblMap->insert(std::make_pair(INDX_UINT_IN_MEM_SHARED_DATA_BUFFERS, &lbl_inMemSharedDataBuffers));
    lblMap->insert(std::make_pair(INDX_UINT_TOTAL_SHARED_DATA_BUFFERS, &lbl_totalSharedDataBuffers));
    lblMap->insert(std::make_pair(INDX_UINT_ABANDONED_BUFFERS, &lbl_abandonedBuffers));
    lblMap->insert(std::make_pair(INDX_UINT_PB_NO_SERIAL_MEM_AVAIL, &lbl_pbNoSerialMemAvail));
    lblMap->insert(std::make_pair(INDX_UINT_CASSANDRA_DATA_INSERT_ERR, &lbl_cassandraDataInsertError));
    lblMap->insert(std::make_pair(INDX_UINT_CASSANDRA_METADATA_INSERT_ERR, &lbl_cassandraMetadataInsertError));
    lblMap->insert(std::make_pair(INDX_UINT_CASSANDRA_ANNOTATIONS_INSERT_ERR, &lbl_cassandraAnnotationsInsertError));
    lblMap->insert(std::make_pair(INDX_UINT_INSERT_ABANDON_NO_CASSANDRA_CONN, &lbl_insertAbandonedNoCassandraConn));
    lblMap->insert(std::make_pair(INDX_STR_DATA_DISCOVERY_MAJ_VER, &lbl_dataDiscoveryMajVer));
    lblMap->insert(std::make_pair(INDX_STR_DATA_DISCOVERY_MIN_VER, &lbl_dataDiscoveryMinVer));
    
    /* next the values */
    std::map<unsigned int, boost::any> * valMap = 
        new std::map<unsigned int, boost::any>();
    valMap->insert(std::make_pair(INDX_UINT_BUF_FMT_ERROR, &val_bufFmtError));
    valMap->insert(std::make_pair(INDX_UINT_IN_MEM_SHARED_DATA_BUFFERS, &sharedInMemDataBufferCnt));
    valMap->insert(std::make_pair(INDX_UINT_TOTAL_SHARED_DATA_BUFFERS, &sharedTotalDataBufferCnt));
    valMap->insert(std::make_pair(INDX_UINT_ABANDONED_BUFFERS, &val_abandonedBuffers));
    valMap->insert(std::make_pair(INDX_UINT_PB_NO_SERIAL_MEM_AVAIL, &val_pbNoSerialMemAvail));
    valMap->insert(std::make_pair(INDX_UINT_CASSANDRA_DATA_INSERT_ERR, &val_cassandraDataInsertError));
    valMap->insert(std::make_pair(INDX_UINT_CASSANDRA_METADATA_INSERT_ERR, &val_cassandraMetadataInsertError));
    valMap->insert(std::make_pair(INDX_UINT_CASSANDRA_ANNOTATIONS_INSERT_ERR, &val_cassandraAnnotationsInsertError));
    valMap->insert(std::make_pair(INDX_UINT_INSERT_ABANDON_NO_CASSANDRA_CONN, &val_insertAbandonedNoCassandraConn));
    valMap->insert(std::make_pair(INDX_STR_DATA_DISCOVERY_MAJ_VER, &val_dataDiscoveryMajVer));
    valMap->insert(std::make_pair(INDX_STR_DATA_DISCOVERY_MIN_VER, &val_dataDiscoveryMinVer));
    
    
    /* init any values */
    val_bufFmtError = 0;
    val_abandonedBuffers = 0;
    val_pbNoSerialMemAvail = 0;
    val_cassandraDataInsertError = 0;
    val_cassandraMetadataInsertError = 0;
    val_cassandraAnnotationsInsertError = 0;
    val_insertAbandonedNoCassandraConn = 0;
    servApi->serviceSpecStatsLabels = lblMap;
    servApi->serviceSpecStatsValues = valMap;
    (servApi->serviceSpecStatsMutex).unlock();
}

void destroyServiceSpecificStats(serviceInstanceApi * servApi) {
    (servApi->serviceSpecStatsMutex).lock();
    /* first the labels */
    delete servApi->serviceSpecStatsLabels;
    
    /* next the values */
    delete servApi->serviceSpecStatsValues; 
    
    servApi->serviceSpecStatsLabels = nullptr;
    servApi->serviceSpecStatsValues = nullptr;
    (servApi->serviceSpecStatsMutex).unlock();
}

void clearServiceSpecificStats(serviceInstanceApi * servApi) {
    (servApi->serviceSpecStatsMutex).lock();
    
    /* init any values */
    val_bufFmtError = 0;
    val_abandonedBuffers = 0;
    val_pbNoSerialMemAvail = 0;
    val_cassandraDataInsertError = 0;
    val_cassandraMetadataInsertError = 0;
    val_cassandraAnnotationsInsertError = 0;
    val_insertAbandonedNoCassandraConn = 0;
    
    (servApi->serviceSpecStatsMutex).unlock();
}

void populateServiceSpecificProtobufStatsList(serviceInstanceApi * servApi,
        KeyValueList * list) {
    
    (servApi->serviceSpecStatsMutex).lock(); 
    list->set_category("Service_Specific_Stats");
    
    for(int i = BASE_INDX_STR_DATA_DISCOVERY_STATS; 
            i < BASE_INDX_STR_DATA_DISCOVERY_STATS + NUM_INDX_STR_DATA_DISCOVERY_STATS;
            i++) {
            KeyValue * pair = list->add_element();
            /* TODO may need special handling for inMem and total shared data
             buffers counts.  They are atomic types. */
            pair->set_key((*(servApi->serviceSpecStatsLabels))[i]->c_str()); 
            pair->set_stringval(any_cast<std::string *>
                ((*(servApi->serviceSpecStatsValues))[i])->c_str());
    }
    
    for(int i = BASE_INDX_UINT_DATA_DISCOVERY_STATS; 
            i < BASE_INDX_UINT_DATA_DISCOVERY_STATS + NUM_INDX_UINT_DATA_DISCOVERY_STATS;
            i++) {
            KeyValue * pair = list->add_element();
            pair->set_key((*(servApi->serviceSpecStatsLabels))[i]->c_str()); 
            pair->set_uintegerval(*(any_cast<uint32_t *>                                                                                                               
                ((*(servApi->serviceSpecStatsValues))[i])));
    }
    
    for(int i = BASE_INDX_INT_DATA_DISCOVERY_STATS; 
            i < BASE_INDX_INT_DATA_DISCOVERY_STATS + NUM_INDX_INT_DATA_DISCOVERY_STATS;
            i++) {
            KeyValue * pair = list->add_element();
            pair->set_key((*(servApi->serviceSpecStatsLabels))[i]->c_str()); 
            pair->set_integerval(*(any_cast<int32_t *>
                ((*(servApi->serviceSpecStatsValues))[i])));
    }
    
    for(int i = BASE_INDX_FLT_DATA_DISCOVERY_STATS; 
            i < BASE_INDX_FLT_DATA_DISCOVERY_STATS + NUM_INDX_FLT_DATA_DISCOVERY_STATS;
            i++) {
            KeyValue * pair = list->add_element();
            pair->set_key((*(servApi->serviceSpecStatsLabels))[i]->c_str()); 
            pair->set_floatval(*(any_cast<float *>
                ((*(servApi->serviceSpecStatsValues))[i])));
    }
    
    (servApi->serviceSpecStatsMutex).unlock();
}