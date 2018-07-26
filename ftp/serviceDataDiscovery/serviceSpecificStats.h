/*
 */

/* 
 * File:   serviceCommonStats.h
 * Author: mcp
 *
 * Created on February 6, 2016, 11:12 AM
 */

#ifndef SERVICESPECIFICSTATS_H
#define SERVICESPECIFICSTATS_H

#define DATA_DISCOVERY_MAJOR_VERSION "0.8"
#define DATA_DISCOVERY_MINOR_VERSION "32"

#define BASE_INDX_UINT_DATA_DISCOVERY_STATS 0
#define NUM_INDX_UINT_DATA_DISCOVERY_STATS 9

#define BASE_INDX_INT_DATA_DISCOVERY_STATS 10
#define NUM_INDX_INT_DATA_DISCOVERY_STATS 0

#define BASE_INDX_FLT_DATA_DISCOVERY_STATS 20
#define NUM_INDX_FLT_DATA_DISCOVERY_STATS 0

#define BASE_INDX_STR_DATA_DISCOVERY_STATS 30
#define NUM_INDX_STR_DATA_DISCOVERY_STATS 2

#define INDX_UINT_BUF_FMT_ERROR 0
#define INDX_UINT_ABANDONED_BUFFERS 1
#define INDX_UINT_IN_MEM_SHARED_DATA_BUFFERS 2
#define INDX_UINT_TOTAL_SHARED_DATA_BUFFERS 3
#define INDX_UINT_PB_NO_SERIAL_MEM_AVAIL 4
#define INDX_UINT_CASSANDRA_DATA_INSERT_ERR 5
#define INDX_UINT_CASSANDRA_METADATA_INSERT_ERR 6
#define INDX_UINT_CASSANDRA_ANNOTATIONS_INSERT_ERR 7
#define INDX_UINT_INSERT_ABANDON_NO_CASSANDRA_CONN 8

#define INDX_STR_DATA_DISCOVERY_MAJ_VER 30
#define INDX_STR_DATA_DISCOVERY_MIN_VER 31

#define LBL_DATA_DISCOVERY_MAJ_VER "Data_Discovery_Major_Version"
#define LBL_DATA_DISCOVERY_MIN_VER "Data_Discovery_Minor_Version"

#define LBL_BUF_FMT_ERROR "Buffer_Format_Error"
#define LBL_IN_MEM_SHARED_DATA_BUFFERS "In_Mem_Shared_Data_Buffers"
#define LBL_TOTAL_SHARED_DATA_BUFFERS "Total_Shared_Data_Buffers"
#define LBL_ABANDONED_BUFFERS "Abandoned_Buffers"
#define LBL_PB_NO_SERIAL_MEM_AVAIL "Protocol_Buffer_No_Serial_Memory_Avail"
#define LBL_CASSANDRA_DATA_INSERT_ERR "Cassandra_Data_Insert_Error"
#define LBL_CASSANDRA_METADATA_INSERT_ERR "Cassandra_Metadata_Insert_Error"
#define LBL_CASSANDRA_ANNOTATIONS_INSERT_ERR "Cassandra_Annotations_Insert_Error"
#define LBL_INSERT_ABANDON_NO_CASSANDRA_CONN "Cassandra_Insert_Abandoned_No_Conn"

void createServiceSpecificStats(serviceCommon::serviceInstanceApi *);

void destroyServiceSpecificStats(serviceCommon::serviceInstanceApi *);

void clearServiceSpecificStats(serviceCommon::serviceInstanceApi *);

void populateServiceSpecificProtobufStatsList(serviceInstanceApi *, KeyValueList *);


#endif /* SERVICESPECIFICSTATS_H */

