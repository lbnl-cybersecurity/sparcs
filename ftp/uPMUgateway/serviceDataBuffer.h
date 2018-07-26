/*
 */

/* 
 * File:   sharedDataBuffer.h
 * Author: mcp
 *
 * Created on February 6, 2016, 7:46 AM
 */

#ifndef SERVICEDATABUFFER_H
#define SERVICEDATABUFFER_H

#include <atomic>

enum dataBufferOffer_t {BUFFER_ACCEPTED = 0, BUFFER_RELEASED = 1, BUFFER_REFUSED = 2};

namespace serviceCommon {
using namespace std;

class serviceDataBuffer {
private:
    std::atomic<unsigned int> * refCount;
    
public:
    serviceDataBuffer(std::atomic<unsigned int> *refCnt,
        std::atomic<unsigned int> *totalCnt) {
        std::cout << "serviceDataBuffer:: no buffer" << std::endl;
        dbRecordIndex = 0;
        refCount = refCnt;
        /* count references and total buffers shared */
        (*refCount)++;
        (*totalCnt)++;
    }
    serviceDataBuffer(std::atomic<unsigned int> *refCnt,
        std::atomic<unsigned int> *totalCnt,
        unsigned int bLen, unsigned char * buf) {
        std::cout << "serviceDataBuffer:: allocate buffer" << std::endl;
        byteLen = bLen;
        dataBuffer = buf;
        dbRecordIndex = 0;
        refCount = refCnt;
        /* count references and total buffers shared */
        (*refCount)++;
        (*totalCnt)++;
    }
    ~serviceDataBuffer() {
        if(dataBuffer != NULL) {
            std::cout << "~serviceDataBuffer:: free buffer" << std::endl;
            try{
            std::free(dataBuffer);
            }
            catch (const std::exception &e) {
                /* log problem */
                std::string s = "~serviceDataBuffer:: free buffer FAILURE";
                s.append(e.what());
                std::cout<< s << std::endl;
                
            }
        }
        else {
            std::cout << "~serviceDataBuffer:: no buffer" << std::endl;
        }
        /* track buffers freed */
        (*refCount)--;
    }
    unsigned char serviceDomain;
    unsigned char serviceSpecific;
    unsigned int dbRecordIndex;
    unsigned int byteLen;
    unsigned char * dataBuffer;
};
}
#endif /* SERVICEDATABUFFER_H */

