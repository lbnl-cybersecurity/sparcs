/*
 */

/* 
 * File:   oneSecDataFormat.h
 * Author: mcp
 *
 * Created on December 25, 2017, 8:37 PM
 */

#ifndef ONESECDATAFORMAT_H
#define ONESECDATAFORMAT_H

/* field offsets (bytes) for repackaging C37 data into 1 sec. buffers */
#define SECBLK_LEN 0
#define SECBLK_LOCK 2
#define SECBLK_TIME 4
#define SECBLK_FRACSEC 8
#define SECBLK_DATA_START 12
#define SECBLK_L1_ANG 12
#define SECBLK_L1_MAG 16
#define SECBLK_L2_ANG 20
#define SECBLK_L2_MAG 24
#define SECBLK_L3_ANG 28
#define SECBLK_L3_MAG 32
#define SECBLK_C1_ANG 36
#define SECBLK_C1_MAG 40
#define SECBLK_C2_ANG 44
#define SECBLK_C2_MAG 48
#define SECBLK_C3_ANG 52
#define SECBLK_C3_MAG 56
#define SECBLK_LEN_BYTES (SECBLK_C3_MAG + 4)


#endif /* ONESECDATAFORMAT_H */

