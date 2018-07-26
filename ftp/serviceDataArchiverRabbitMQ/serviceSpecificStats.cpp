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

using namespace boost;
using namespace serviceCommon;

#include "serviceSpecificStats.h"

/* these need to be static */
/* label storage */
static std::string lbl_bufFmtError(LBL_BUF_FMT_ERROR);
static std::string lbl_abandonedBuffers(LBL_ABANDONED_BUFFERS);
static std::string lbl_pbNoSerialMemAvail(LBL_PB_NO_SERIAL_MEM_AVAIL);
static std::string lbl_rabbitmqDataInsertError(LBL_RABBITMQ_DATA_INSERT_ERR);
static std::string lbl_insertAbandonedNoRabbitmqConn(LBL_INSERT_ABANDON_NO_RABBITMQ_CONN);
static std::string lbl_minAvgDataInsertMsec(LBL_MIN_AVG_DATA_INSERT_MSEC);
static std::string lbl_maxAvgDataInsertMsec(LBL_MAX_AVG_DATA_INSERT_MSEC);
static std::string lbl_lastAvgDataInsertMsec(LBL_LAST_AVG_DATA_INSERT_MSEC);
static std::string lbl_dataArchiverRabbitmqMajVer(LBL_DATA_ARCHIVER_RABBITMQ_MAJ_VER);
static std::string lbl_dataArchiverRabbitmqMinVer(LBL_DATA_ARCHIVER_RABBITMQ_MIN_VER);

/* value storage **/
static unsigned int val_bufFmtError = 0;
static unsigned int val_abandonedBuffers = 0;
static unsigned int val_pbNoSerialMemAvail = 0;
static unsigned int val_rabbitmqDataInsertError = 0;
static unsigned int val_insertAbandonedNoRabbitmqConn = 0;
static float val_minAvgDataInsertMsec = 0.0;
static float val_maxAvgDataInsertMsec = 0.0;
static float val_lastAvgDataInsertMsec = 0.0;
static std::string val_dataArchiverRabbitmqMajVer(DATA_ARCHIVER_RABBITMQ_MAJOR_VERSION);
static std::string val_dataArchiverRabbitmqMinVer(DATA_ARCHIVER_RABBITMQ_MINOR_VERSION);

void createServiceSpecificStats(serviceInstanceApi * servApi) {
    (servApi->serviceSpecStatsMutex).lock();
    /* first the labels */
    std::map<unsigned int, std::string *> * lblMap = 
        new std::map<unsigned int, std::string *>();
    lblMap->insert(std::make_pair(INDX_UINT_BUF_FMT_ERROR, &lbl_bufFmtError));
    lblMap->insert(std::make_pair(INDX_UINT_ABANDONED_BUFFERS, &lbl_abandonedBuffers));
    lblMap->insert(std::make_pair(INDX_UINT_PB_NO_SERIAL_MEM_AVAIL, &lbl_pbNoSerialMemAvail));
    lblMap->insert(std::make_pair(INDX_UINT_RABBITMQ_DATA_INSERT_ERR, &lbl_rabbitmqDataInsertError));
    lblMap->insert(std::make_pair(INDX_UINT_INSERT_ABANDON_NO_RABBITMQ_CONN, &lbl_insertAbandonedNoRabbitmqConn));
    lblMap->insert(std::make_pair(INDX_FLT_MIN_AVG_DATA_INSERT_MSEC, &lbl_minAvgDataInsertMsec));
    lblMap->insert(std::make_pair(INDX_FLT_MAX_AVG_DATA_INSERT_MSEC, &lbl_maxAvgDataInsertMsec));
    lblMap->insert(std::make_pair(INDX_FLT_LAST_AVG_DATA_INSERT_MSEC, &lbl_lastAvgDataInsertMsec));
    lblMap->insert(std::make_pair(INDX_STR_DATA_ARCHIVER_RABBITMQ_MAJ_VER, &lbl_dataArchiverRabbitmqMajVer));
    lblMap->insert(std::make_pair(INDX_STR_DATA_ARCHIVER_RABBITMQ_MIN_VER, &lbl_dataArchiverRabbitmqMinVer));
    
    /* next the values */
    std::map<unsigned int, boost::any> * valMap = 
        new std::map<unsigned int, boost::any>();
    valMap->insert(std::make_pair(INDX_UINT_BUF_FMT_ERROR, &val_bufFmtError));
    valMap->insert(std::make_pair(INDX_UINT_ABANDONED_BUFFERS, &val_abandonedBuffers));
    valMap->insert(std::make_pair(INDX_UINT_PB_NO_SERIAL_MEM_AVAIL, &val_pbNoSerialMemAvail));
    valMap->insert(std::make_pair(INDX_UINT_RABBITMQ_DATA_INSERT_ERR, &val_rabbitmqDataInsertError));
    valMap->insert(std::make_pair(INDX_UINT_INSERT_ABANDON_NO_RABBITMQ_CONN, &val_insertAbandonedNoRabbitmqConn));
    valMap->insert(std::make_pair(INDX_FLT_MIN_AVG_DATA_INSERT_MSEC, &val_minAvgDataInsertMsec));
    valMap->insert(std::make_pair(INDX_FLT_MAX_AVG_DATA_INSERT_MSEC, &val_maxAvgDataInsertMsec));
    valMap->insert(std::make_pair(INDX_FLT_LAST_AVG_DATA_INSERT_MSEC, &val_lastAvgDataInsertMsec));
    valMap->insert(std::make_pair(INDX_STR_DATA_ARCHIVER_RABBITMQ_MAJ_VER, &val_dataArchiverRabbitmqMajVer));
    valMap->insert(std::make_pair(INDX_STR_DATA_ARCHIVER_RABBITMQ_MIN_VER, &val_dataArchiverRabbitmqMinVer));
    
    
    /* init any values */
    val_bufFmtError = 0;
    val_abandonedBuffers = 0;
    val_pbNoSerialMemAvail = 0;
    val_rabbitmqDataInsertError = 0;
    val_insertAbandonedNoRabbitmqConn = 0;
    val_minAvgDataInsertMsec = 0.0;
    val_maxAvgDataInsertMsec = 0.0;
    val_lastAvgDataInsertMsec = 0.0;
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
    val_rabbitmqDataInsertError = 0;
    val_insertAbandonedNoRabbitmqConn = 0;
    val_minAvgDataInsertMsec = 0.0;
    val_maxAvgDataInsertMsec = 0.0;
    val_lastAvgDataInsertMsec = 0.0;
    
    (servApi->serviceSpecStatsMutex).unlock();
}

void populateServiceSpecificProtobufStatsList(serviceInstanceApi * servApi,
        KeyValueList * list) {
    
    (servApi->serviceSpecStatsMutex).lock(); 
    list->set_category("Service_Specific_Stats");
    
    for(int i = BASE_INDX_STR_DATA_ARCHIVER_RABBITMQ_STATS; 
            i < BASE_INDX_STR_DATA_ARCHIVER_RABBITMQ_STATS + NUM_INDX_STR_DATA_ARCHIVER_RABBITMQ_STATS;
            i++) {
            KeyValue * pair = list->add_element();
            pair->set_key((*(servApi->serviceSpecStatsLabels))[i]->c_str()); 
            pair->set_stringval(any_cast<std::string *>
                ((*(servApi->serviceSpecStatsValues))[i])->c_str());
    }
    
    for(int i = BASE_INDX_UINT_DATA_ARCHIVER_RABBITMQ_STATS; 
            i < BASE_INDX_UINT_DATA_ARCHIVER_RABBITMQ_STATS + NUM_INDX_UINT_DATA_ARCHIVER_RABBITMQ_STATS;
            i++) {
            KeyValue * pair = list->add_element();
            pair->set_key((*(servApi->serviceSpecStatsLabels))[i]->c_str()); 
            pair->set_uintegerval(*(any_cast<uint32_t *>                                                                                                               
                ((*(servApi->serviceSpecStatsValues))[i])));
    }
    
    for(int i = BASE_INDX_INT_DATA_ARCHIVER_RABBITMQ_STATS; 
            i < BASE_INDX_INT_DATA_ARCHIVER_RABBITMQ_STATS + NUM_INDX_INT_DATA_ARCHIVER_RABBITMQ_STATS;
            i++) {
            KeyValue * pair = list->add_element();
            pair->set_key((*(servApi->serviceSpecStatsLabels))[i]->c_str()); 
            pair->set_integerval(*(any_cast<int32_t *>
                ((*(servApi->serviceSpecStatsValues))[i])));
    }
    
    for(int i = BASE_INDX_FLT_DATA_ARCHIVER_RABBITMQ_STATS; 
            i < BASE_INDX_FLT_DATA_ARCHIVER_RABBITMQ_STATS + NUM_INDX_FLT_DATA_ARCHIVER_RABBITMQ_STATS;
            i++) {
            KeyValue * pair = list->add_element();
            pair->set_key((*(servApi->serviceSpecStatsLabels))[i]->c_str()); 
            pair->set_floatval(*(any_cast<float *>
                ((*(servApi->serviceSpecStatsValues))[i])));
    }
    
    (servApi->serviceSpecStatsMutex).unlock();
}