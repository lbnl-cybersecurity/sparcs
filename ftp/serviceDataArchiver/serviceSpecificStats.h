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

#define DATA_ARCHIVER_MAJOR_VERSION "0.9"
#define DATA_ARCHIVER_MINOR_VERSION "32"

#define BASE_INDX_UINT_DATA_ARCHIVER_STATS 0
#define NUM_INDX_UINT_DATA_ARCHIVER_STATS 7

#define BASE_INDX_INT_DATA_ARCHIVER_STATS 10
#define NUM_INDX_INT_DATA_ARCHIVER_STATS 0

#define BASE_INDX_FLT_DATA_ARCHIVER_STATS 20
#define NUM_INDX_FLT_DATA_ARCHIVER_STATS 3

#define BASE_INDX_STR_DATA_ARCHIVER_STATS 30
#define NUM_INDX_STR_DATA_ARCHIVER_STATS 2

#define INDX_UINT_BUF_FMT_ERROR 0
#define INDX_UINT_ABANDONED_BUFFERS 1
#define INDX_UINT_PB_NO_SERIAL_MEM_AVAIL 2
#define INDX_UINT_CASSANDRA_DATA_INSERT_ERR 3
#define INDX_UINT_CASSANDRA_METADATA_INSERT_ERR 4
#define INDX_UINT_CASSANDRA_ANNOTATIONS_INSERT_ERR 5
#define INDX_UINT_INSERT_ABANDON_NO_CASSANDRA_CONN 6

#define INDX_FLT_MIN_AVG_DATA_INSERT_MSEC 20
#define INDX_FLT_MAX_AVG_DATA_INSERT_MSEC 21
#define INDX_FLT_LAST_AVG_DATA_INSERT_MSEC 22

#define INDX_STR_DATA_ARCHIVER_MAJ_VER 30
#define INDX_STR_DATA_ARCHIVER_MIN_VER 31

#define LBL_DATA_ARCHIVER_MAJ_VER "Data_Archiver_Major_Version"
#define LBL_DATA_ARCHIVER_MIN_VER "Data_Archiver_Minor_Version"

#define LBL_BUF_FMT_ERROR "Buffer_Format_Error"
#define LBL_ABANDONED_BUFFERS "Abandoned_Buffers"
#define LBL_PB_NO_SERIAL_MEM_AVAIL "Protocol_Buffer_No_Serial_Memory_Avail"
#define LBL_CASSANDRA_DATA_INSERT_ERR "Cassandra_Data_Insert_Error"
#define LBL_CASSANDRA_METADATA_INSERT_ERR "Cassandra_Metadata_Insert_Error"
#define LBL_CASSANDRA_ANNOTATIONS_INSERT_ERR "Cassandra_Annotations_Insert_Error"
#define LBL_INSERT_ABANDON_NO_CASSANDRA_CONN "Cassandra_Insert_Abandoned_No_Conn"
#define LBL_MIN_AVG_DATA_INSERT_MSEC "Minimum_Average_Data_Insert_Time_Msec"
#define LBL_MAX_AVG_DATA_INSERT_MSEC "Maximum_Average_Data_Insert_Time_Msec"
#define LBL_LAST_AVG_DATA_INSERT_MSEC "Last_Average_Data_Insert_Time_Msec"

void createServiceSpecificStats(serviceCommon::serviceInstanceApi *);

void destroyServiceSpecificStats(serviceCommon::serviceInstanceApi *);

void clearServiceSpecificStats(serviceCommon::serviceInstanceApi *);

void populateServiceSpecificProtobufStatsList(serviceInstanceApi *, KeyValueList *);


#endif /* SERVICESPECIFICSTATS_H */

