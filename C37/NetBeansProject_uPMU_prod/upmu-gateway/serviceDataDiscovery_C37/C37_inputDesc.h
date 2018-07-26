/*
 */

/* 
 * File:   C37_inputSource.h
 * Author: mcp
 *
 * Created on October 29, 2017, 12:23 AM
 */

#ifndef C37_INPUTDESC_H
#define C37_INPUTDESC_H

/* define buffer for largest allowable C37 data event */
#define C37_DATUM_TYPICAL_LEN 74
#define C37_DATUM_MAX_LEN 100
#define C37_NUM_DATA_BUFFERS 200
typedef uint8_t C37_DATUM_T [C37_DATUM_MAX_LEN];

#endif /* C37_INPUTDESC_H */

