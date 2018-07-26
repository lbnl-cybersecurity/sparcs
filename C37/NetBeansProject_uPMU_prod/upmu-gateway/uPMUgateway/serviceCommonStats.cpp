/*

 */


#include <mutex>
#include <map>
#include <ctime>
#include <boost/thread/condition.hpp>
#include <boost/any.hpp>

#include "serviceCommandQueueTemplate.h"
#include "serviceInstanceApi.h"
#include "upmuStructKeyValueListProtobuf.pb.h"

namespace serviceCommon {
using namespace boost;


#include "serviceCommonStats.h"


/* these need to be static */
/* label storage */
static std::string lbl_serviceStartTime(LBL_SERVICE_START_TIME);
static std::string lbl_numDataBufSeen(LBL_NUM_DATA_BUF_SEEN);
static std::string lbl_numDataBufAccepted(LBL_NUM_DATA_BUF_ACCEPTED);
static std::string lbl_numDataBufRejected(LBL_NUM_DATA_BUF_REJECTED);

/* value storage **/
static unsigned int val_serviceStartTime = 0;
static unsigned int val_numDataBufSeen = 0;
static unsigned int val_numDataBufAccepted = 0;
static unsigned int val_numDataBufRejected = 0;

void createServiceCommonStats(serviceInstanceApi * servApi) {
    (servApi->serviceCmnStatsMutex).lock();
    /* first the labels */
    std::map<unsigned int, std::string *> * lblMap = 
        new std::map<unsigned int, std::string *>();
    lblMap->insert(std::make_pair(INDX_UINT_SERVICE_START_TIME, &lbl_serviceStartTime));
    lblMap->insert(std::make_pair(INDX_UINT_NUM_DATA_BUF_SEEN, &lbl_numDataBufSeen));
    lblMap->insert(std::make_pair(INDX_UINT_NUM_DATA_BUF_ACCEPTED, &lbl_numDataBufAccepted));
    lblMap->insert(std::make_pair(INDX_UINT_NUM_DATA_BUF_REJECTED, &lbl_numDataBufRejected));
    
    /* next the values */
    std::map<unsigned int, boost::any> * valMap = 
        new std::map<unsigned int, boost::any>();
    std::time_t result = std::time(nullptr);
    val_serviceStartTime = (unsigned int)result;
    valMap->insert(std::make_pair(INDX_UINT_SERVICE_START_TIME, &val_serviceStartTime));
    valMap->insert(std::make_pair(INDX_UINT_NUM_DATA_BUF_SEEN, &val_numDataBufSeen));
    valMap->insert(std::make_pair(INDX_UINT_NUM_DATA_BUF_ACCEPTED, &val_numDataBufAccepted));
    valMap->insert(std::make_pair(INDX_UINT_NUM_DATA_BUF_REJECTED, &val_numDataBufRejected));
    
    /* init any values */
    val_serviceStartTime = (uint32_t)time(NULL);
    val_numDataBufSeen = 0;
    val_numDataBufAccepted = 0;
    val_numDataBufRejected = 0;
    
    servApi->serviceCmnStatsLabels = lblMap;
    servApi->serviceCmnStatsValues = valMap; 
    (servApi->serviceCmnStatsMutex).unlock();
}

void destroyServiceCommonStats(serviceInstanceApi * servApi) {
    servApi->serviceCmnStatsMutex.lock();
    /* first the labels */
    delete servApi->serviceCmnStatsLabels;
    
    /* next the values */
    delete servApi->serviceCmnStatsValues;
    
    servApi->serviceCmnStatsLabels = nullptr;
    servApi->serviceCmnStatsValues = nullptr;
    (servApi->serviceCmnStatsMutex).unlock();    
}

void clearServiceCommonStats(serviceInstanceApi * servApi) {
    (servApi->serviceCmnStatsMutex).lock();
    
    /* init any values */
    val_numDataBufSeen = 0;
    val_numDataBufAccepted = 0;
    val_numDataBufRejected = 0;
    
    (servApi->serviceCmnStatsMutex).unlock();
}

void populateServiceCommonProtobufStatsList(serviceInstanceApi * servApi,
        KeyValueList * list) {
    
    (servApi->serviceCmnStatsMutex).lock(); 
    list->set_category("Service_Common_Stats");
    
    for(int i = BASE_INDX_STR_COMMON_STATS; 
            i < BASE_INDX_STR_COMMON_STATS + NUM_INDX_STR_COMMON_STATS;
            i++) {
            KeyValue * pair = list->add_element();
            pair->set_key((*(servApi->serviceSpecStatsLabels))[i]->c_str()); 
            pair->set_stringval(any_cast<std::string *>
                ((*(servApi->serviceSpecStatsValues))[i])->c_str());
    }
    
    for(int i = BASE_INDX_UINT_COMMON_STATS; 
            i < BASE_INDX_UINT_COMMON_STATS + NUM_INDX_UINT_COMMON_STATS;
            i++) {
            KeyValue * pair = list->add_element();
            pair->set_key((*(servApi->serviceSpecStatsLabels))[i]->c_str()); 
            pair->set_uintegerval(*(any_cast<uint32_t *>                                                                                                               
                ((*(servApi->serviceSpecStatsValues))[i])));
    }
    
    for(int i = BASE_INDX_INT_COMMON_STATS; 
            i < BASE_INDX_INT_COMMON_STATS + NUM_INDX_INT_COMMON_STATS;
            i++) {
            KeyValue * pair = list->add_element();
            pair->set_key((*(servApi->serviceSpecStatsLabels))[i]->c_str()); 
            pair->set_integerval(*(any_cast<int32_t *>
                ((*(servApi->serviceSpecStatsValues))[i])));
    }
    
    for(int i = BASE_INDX_FLT_COMMON_STATS; 
            i < BASE_INDX_FLT_COMMON_STATS + NUM_INDX_FLT_COMMON_STATS;
            i++) {
            KeyValue * pair = list->add_element();
            pair->set_key((*(servApi->serviceSpecStatsLabels))[i]->c_str()); 
            pair->set_floatval(*(any_cast<float *>
                ((*(servApi->serviceSpecStatsValues))[i])));
    }
    
    (servApi->serviceCmnStatsMutex).unlock();
}
}
