/*
 */

/* 
 * File:   serviceCommonStats.h
 * Author: mcp
 *
 * Created on February 6, 2016, 11:12 AM
 */

#ifndef SERVICECOMMONSTATS_H
#define SERVICECOMMONSTATS_H

#define BASE_INDX_UINT_COMMON_STATS 0
#define NUM_INDX_UINT_COMMON_STATS 4

#define BASE_INDX_INT_COMMON_STATS 10
#define NUM_INDX_INT_COMMON_STATS 0

#define BASE_INDX_FLT_COMMON_STATS 20
#define NUM_INDX_FLT_COMMON_STATS 0

#define BASE_INDX_STR_COMMON_STATS 30
#define NUM_INDX_STR_COMMON_STATS 0

#define INDX_UINT_SERVICE_START_TIME 0
#define INDX_UINT_NUM_DATA_BUF_SEEN 1
#define INDX_UINT_NUM_DATA_BUF_ACCEPTED 2
#define INDX_UINT_NUM_DATA_BUF_REJECTED 3


#define LBL_SERVICE_START_TIME "Service_Start_Timestamp_GMT"
#define LBL_NUM_DATA_BUF_SEEN "Num_Data_Buffers_Seen"
#define LBL_NUM_DATA_BUF_ACCEPTED "Num_Data_Buffers_Accepted"
#define LBL_NUM_DATA_BUF_REJECTED "Num_Data_Buffers_Rejected"

namespace serviceCommon {
    
void createServiceCommonStats(serviceInstanceApi *);

void destroyServiceCommonStats(serviceInstanceApi *);

void clearServiceCommonStats(serviceInstanceApi *);

void populateServiceCommonProtobufStatsList(serviceInstanceApi *, KeyValueList *);
}

#endif /* SERVICECOMMONSTATS_H */

