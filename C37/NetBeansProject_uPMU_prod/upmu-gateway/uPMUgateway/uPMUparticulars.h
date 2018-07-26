/*
 */

/* 
 * File:   uPMUparticulars.h
 * Author: mcp
 *
 * Created on February 17, 2016, 10:14 AM
 */

#ifndef UPMUPARTICULARS_H
#define UPMUPARTICULARS_H

/* current fixed size of uPMU 1 second data buffer with 120 samples.
 note: this differs from size of data struct by 44 bytes. */
#define UPMU_1SEC_BUF_LEN 6312

#define UPMU_SAMPLES_PER_SEC 120

/* uPMU binary data strutures */

typedef struct     {
     unsigned int year;
     unsigned int month;
     unsigned int day;
     unsigned int hour;
     unsigned int min;
     unsigned int sec;
} time_linux;
  
typedef struct {
     float angle;
     float mag;
}sync_point;
  
typedef struct {
     float sampleRate;
     time_linux sectionTime;
     int lockstate[120];
     sync_point L1MagAng[120];
     sync_point L2MagAng[120];
     sync_point L3MagAng[120];
     sync_point C1MagAng[120];
     sync_point C2MagAng[120];
     sync_point C3MagAng[120];
}sync_output;

#endif /* UPMUPARTICULARS_H */

