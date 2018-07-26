/*
 * Stream-Processing Architecture for Real-time Cyber-physical Security (SPARCS)
 */


/* 
 * File:   C37_ReFormat.cpp
 * Author: mcp
 * 
 * Created on December 8, 2017, 11:26 PM
 */
#include <atomic>
#include <vector>
#include <iostream>
#include <memory>

#include <pqxx/pqxx>
#include "cassandra.h"
#include <curl/curl.h>

/* boost includes */
#include <boost/any.hpp>
#include <boost/asio.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/circular_buffer/base.hpp>

/* Json library */
//#include <jsoncpp/json/json.h>

#include <log4cpp/Appender.hh>
#include <log4cpp/Category.hh>
#include <log4cpp/FileAppender.hh>
#include <log4cpp/OstreamAppender.hh>
#include <log4cpp/Layout.hh>
#include <log4cpp/BasicLayout.hh>
#include <log4cpp/Priority.hh>

#include "dbConnection.h"
#include "domainAndMessageTypes.h"
#include "serviceCommand.h"
#include "serviceCommandQueueTemplate.h"
#include "serviceInstanceApi.h"
#include "serviceInstance.h"
#include "serviceInstanceList.h"
#include "serviceStatus.h"
#include "serviceDataBuffer.h"
#include "serviceOperationsLoggerPostgres.h"
#include "serviceOperationsLoggerCassandra.h"
#include "upmuFtpAccess.h"
#include "upmuStructKeyValueListProtobuf.pb.h"

using namespace serviceCommon;
using namespace serviceDataDiscovery;

#include "serviceCommonStats.h"
#include "serviceSpecificStats.h"
#include "uPMUgmtime.h"
#include "serviceDataDiscovery.h"

#include "C37_inputDesc.h"
#include "C37_Buffer.h"
#include "uPMUparticulars.h"
#include "C37_MsgFormats.h"
#include "oneSecDataFormat.h"
#include "C37_Reformat.h"

C37_Reformat::C37_Reformat() {
}

C37_Reformat::~C37_Reformat() {
}

void C37_Reformat::processC37event(uint32_t fracSec, 
    uint32_t sec, C37_Buffer * C37buf) {
    
    /*testing*/
    //std::cout<<"This is C37_Buffer->buffer" << std::hex<<C37buf->buffer;
    
    //std::cout<< "time found is: "<< sec << std::endl;
    
    if(C37buf->msgLenBytes != C37_DATUM_TYPICAL_LEN) {
        /* this is not a normal C37 event.  Count it, but do
         not process it. */
        nonStdC37events++;
        return;
    }
    switch (currentState) {
        case syncToSec: {
            if(!syncStarted) {
                /* if we get frcSec == 0, start the sync second accumulation */
                if(fracSec == 0) {
                    syncStarted = true;
                    fracSecValues[0] = 0;               
                    presentSec = sec;
                    sampleCnt = 1;
                }
            }
            else {
                if((sec = presentSec + 1) && (fracSec == 0)){
                    /* sanity check for healthy syncSec accumulation.  If sample
                     counts mod(10) (e.g. 120), we consider syncSec successful and
                     start filling buffers. */
                    if(sampleCnt%10 == 0) {
                        sampleCntValid = true;
                        currentState = fillBuffer;
                        C37eventBuf.clear();
                        saveC37event(C37buf);
                        currentSampleCnt = 1;
                        previousSec = presentSec;
                        presentSec = sec;
                        std::cout << "Move to fillBuffer." << std::endl;
                    }
                    else {
                        /*failed above sanity check.  Go back and start
                         another syncSec. */
                        syncStarted = false;
                        sampleCnt = 0;
                        syncToSecAborts++;
                    }
                }
                else if (sec < presentSec) {
                    /* if sec has progressed too far, we missed a bunch of
                     samples in the syncSec.  Abort and retry.
                     */
                    syncStarted = false;
                    sampleCnt = 0;
                    syncToSecAborts++;
                }
                else {
                    /* save syncSec fracSec values. */
                    fracSecValues[sampleCnt++] = fracSec;
                }
            }
            break;
        }
        case fillBuffer: {
            if((sec == presentSec) && (fracSec != 0)) {
                /* save the event and test if we have received a sample with
                 an incorrect fracSec value.  Take no corrective action. */
                saveC37event(C37buf);
                if(fracSec != fracSecValues[currentSampleCnt]) {
                    std::cout << "Mising sample. " << std::endl;
                }
                currentSampleCnt++;
            }
            else if((sec == presentSec + 1) && (fracSec == 0)) {
                /* Just jumped into next second.  Prepare
                 to assemble 1 sec buffer and enqueue. */
                std::cout << "End of second." << std::endl;
                if(currentSampleCnt != sampleCnt) {
                    missingC37events++;
                }            
                processedC37_1sec_Buffers++;
                // process the saved events
                processEventBuffer();
                /* initialize buffer, add current event and accumulate next 
                 second's worth of events. */
                saveC37event(C37buf);
                currentSampleCnt = 1;
                previousSec = presentSec;
                presentSec = sec;  
            }
            else {
                std::cout << "Total confusion." << std::endl;
                // we missed multiple events and are in a future second
                //save this event
                std::cout << "ERROR";
                 //TODO logger_uPMUgateway.error("C37 error skipping minutes"); 
                
                //saveC37event(C37buf);
                //process saved events
                //processEventBuffer();
                
                int i;
                extendedMissingC37events++;
    
                previousSec = presentSec;
                presentSec = sec;
                // compute correct currentSampleCnt
                for(i = 0; i < sampleCnt; i++) {
                    if(fracSec == fracSecValues[i]) {
                        // save event
                        currentSampleCnt = i + 1;
                        break;
                    }
                }
                if(i == sampleCnt) {
                    //we didnt find a match...something crazy is going on.
                    //resync to seconds.
                    currentState = syncToSec;
                    syncStarted = false;
                    sampleCnt = 0;
                    C37eventSequenceFailures++;
                    return;
                }
            }
            break;
        }
        default: {
            break;
        }
    }
}

void C37_Reformat::saveC37event(C37_Buffer * C37buf) {
    std::unique_ptr<uint8_t []> tBuf = std::make_unique<uint8_t []>(C37buf->msgLenBytes + 4);
    /* copy in event length */
    *(uint64_t *)(&tBuf[0]) = C37buf->msgLenBytes;
    /* copy the event into it */
    std::memcpy(&tBuf[4], (uint8_t *)C37buf->buffer, C37buf->msgLenBytes + 4);
    /* queue it into circular buffer */
    C37eventBuf.push_back(std::move(tBuf));
    
    
}

void C37_Reformat::processEventBuffer() {
    
    
    
    /* get empty buffer */
    uint32_t resultingBufByteLen = (SECBLK_LEN_BYTES * sampleCnt) + 4;
    uint8_t * tBuf = (uint8_t *)std::malloc(resultingBufByteLen);
    uint32_t * p32 = (uint32_t *)(tBuf);
    uint32_t * p33 = (uint32_t *)(tBuf);
    
    *p32++ = resultingBufByteLen;
    for(auto it = C37eventBuf.begin(); it != C37eventBuf.end(); ++it) {
         uint8_t * pin = (it->get());
         /* copy the event into it. Note skip first 4 bytes which is byte count of
          entire buffer. */
         
         uint8_t* damm= pin;
         
//         for(int i = 0; i < resultingBufByteLen; i++) {
//            printf("0x%02x, ", damm[i]);
//        }
//        printf("\n");
//         for (int i=0;i<resultingBufByteLen;i++){
//             std::cout << i <<std::hex << *damm++ << std::endl;
//             
//         }
         
         
         
         uint16_t * p16 = (uint16_t *)p32;
         *p16++ = SECBLK_LEN_BYTES;
         uint8_t l = *(pin + FRACSEC_INDX + 3);
         *p16 = (uint16_t)l; //rm ++
        // std::cout << "p16:" << *p16 << std::endl;
         p16++;
         p32++;
         p32 = (uint32_t *)p16;
         *p32 = *(uint32_t *)(pin + SOC_INDX); //rm ++
         p32++;
         
        // *p32 = (*(uint32_t *)(pin + FRACSEC_INDX) & 0xffffff); //this line is for the epochtime and not for the fracseconds
          *p32 = (*(uint32_t *)(pin + FRACSEC_INDX)); //this is the correct line
        // std::cout << "p322:" << *p32 << std::endl;
         p32++;
        // std::cout << "in: " <<  (*(uint32_t *)(pin + FRACSEC_INDX) & 0xffffff) << " out: " << *p32++ << std::endl;
         for(int i = 0; i < 12; i++) {
             *p32++ = *(uint32_t *)(pin + FIRST_PHASORS_INDX + (i * 4));
              //std::cout << "1in: " << *(uint32_t *)(pin + FIRST_PHASORS_INDX + (i * 4)) << " out: " << *p32++ << std::endl;
         }
         int k = 0;
         
           
//         for(int i = 0; i < resultingBufByteLen; i++) {
//            printf("3x%08x, ", p33[i]);
//            
//        }
//        printf("\n");
         
    } 
    /* offer this buffer as a shared object to other services */
    postProcessUPMUdata(tBuf, 
            (unsigned int)resultingBufByteLen); 
    C37eventBuf.clear();
}

void C37_Reformat::postProcessUPMUdata(unsigned char *dataBuf,
    unsigned int dataLen) {
    int sts;
    
    std::shared_ptr<serviceDataBuffer> 
            ptr(new serviceDataBuffer(&sharedInMemDataBufferCnt, 
            &sharedTotalDataBufferCnt, dataLen, dataBuf));
    /* offer data to other services */
    for (auto si: services->services) {
        if(si.second->serviceAcceptsData== true) { //this is better than the old solution

                dataBufferOffer_t sts =
                    si.second->serviceThread->dataBufferOffer(ptr);
                std::cout << "Discovery Buffer offered, internal counter: " << sharedInMemDataBufferCnt << ", to" << si.first << std::endl;
        }
//        if(si.first.compare("serviceDataDiscovery") != 0) {
//            if(si.first.compare("serviceDataArchiver") == 0) {
//                dataBufferOffer_t sts =
//                    si.second->serviceThread->dataBufferOffer(ptr);
//                std::cout << "Buffer offered, cnt: " << sharedInMemDataBufferCnt << std::endl;
//            }
//        }
    }
    /* release our reference to the shared pointer.  If any other service is
     using it, they have already added their own reference.  It will be destroyed when the
     last reference is released. */
    ptr.reset();
}
