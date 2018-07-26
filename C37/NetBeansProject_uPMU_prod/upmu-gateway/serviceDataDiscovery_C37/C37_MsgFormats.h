/*
 */

/* 
 * File:   C37_MsgFormats.h
 * Author: mcp
 *
 * Created on April 29, 2017, 6:58 PM
 */

#ifndef C37_MSGFORMATS_H
#define C37_MSGFORMATS_H

/* Common to all frames */
#define SYNC_VALUE 0xaa
#define SYNC_INDX 0
#define FRAMESIZE_INDX 2
#define IDCODE_INDX 4
#define SOC_INDX 6
#define FRACSEC_INDX 10

/* Data frames */
#define DATA_MSG_TYPE_VALUE 0
#define STAT_INDX 14
#define FIRST_PHASORS_INDX 16
#define FIRST_FREQ_INDX
#define FIRST_DFREQ_INDX 
#define FIRST_ANALOG_INDX
#define FIRST_DIGITAL_INDX

/* Command frame */
#define HDR_CMD_VALUE 0x3
#define CFG_1_CMD_VALUE 0x4
#define CFG_2_CMD_VALUE 0x5
#define CFG_3_CMD_VALUE 0x6
#define EXTFR_CMD_VALUE 0x8
#define CMD_SYNC_VALUE 0x41
#define CMD_INDX 14
#define EXTFRAME_INDX 16

/* Header frame */
#define HDR_MSG_TYPE_VALUE 1
#define DATA_1_INDX 14

/* Configuration 1 and 2 frames */
#define CFG_1_MSG_TYPE_VALUE 2
#define CFG_1_MSG_BYTE_VALUE 0x21
#define CFG_1_2_MIN_MSG_LEN 54
#define CFG_2_MSG_TYPE_VALUE 3
#define CFG_2_MSG_BYTE_VALUE 0x31
#define TIME_BASE_INDX 14
#define NUM_PMU_INDX 18
#define FIRST_PMU_INDX 20
#define STN_OFFSET 0
#define IDCODE_OFFSET 16
#define FORMAT_OFFSET 18
#define PHNMR_OFFSET 20
#define ANNMR_OFFSET 22
#define DGNMR_OFFSET 24
#define CHNAM_OFFSET 26

#define FIRST_PHUNIT_INDX
#define FIRST_ANUNIT_INDX
#define FIRST_DIGUNIT_INDX
#define FIRST_FNOM_INDX
#define FIRST_CFGCNT_INDX
#define DATA_RATE_INDX 

/* Error return values */
#define ERR_MSG_LARGER_THAN_BUFFER -1
#define ERR_EXAM_BUF_CALL_OUT_OF_SEQ -2
#define ERR_ILLEGAL_BUF_STATE -3
#define ERR_BUF_STATE_MANGLED -4

#endif /* C37_MSGFORMATS_H */

