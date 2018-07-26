/*
 * Stream-Processing Architecture for Real-time Cyber-physical Security (SPARCS)
 */

/* 
 * File:   C37_Buffer.cpp
 * Author: mcp,rgentz
 * 
 * Created on April 24, 2017, 12:51 AM
 */

#include <cstdlib>
#include <boost/asio.hpp>
#include <boost/asio/buffer.hpp>
#include "C37_MsgFormats.h"

using namespace boost::asio;
#include "C37_Buffer.h"

//namespace C37_Util {
    
C37_Buffer::C37_Buffer() {
    //buffer = new unsigned char [DEFAULT_BUFFER_LEN];
    maxBufferBytes = DEFAULT_BUFFER_LEN;
    validBufferBytes = 0;
    bufState = unknownMsg;
    msgLenBytes = 0;
    msgCounter = 0;
}

C37_Buffer::~C37_Buffer() {
    if(buffer != NULL) {
        //delete buffer;
    }
}

void C37_Buffer::init() {
    maxBufferBytes = DEFAULT_BUFFER_LEN;
    validBufferBytes = 0;
    bufState = unknownMsg;
    msgLenBytes = 0;
    msgCounter = 0;
}

void C37_Buffer::setBytesReceived(unsigned int bytesReceived) {
    validBufferBytes += bytesReceived;
}

int C37_Buffer::examineBuffer() {
    switch (bufState) {
        case unknownMsg: {
            if(validBufferBytes >= 4) {
                /* extract message length and frame type  */
                msgLenBytes = buffer[FRAMESIZE_INDX + 1] + 
                        (buffer[FRAMESIZE_INDX] << 8);
                msgType = (buffer[SYNC_INDX + 1] & 0x70) >> 4;
                /* test that we have enough memory to get entire message */
                if(msgLenBytes > maxBufferBytes) {
                    /* we can never receive this message in a single buffer */
                    return ERR_MSG_LARGER_THAN_BUFFER;
                }
                /* indicate buffer has partial message */
                bufState = partialMsg;
            }
            else {
                /* buffer does not have enough data to compute message length */
                return maxBufferBytes - validBufferBytes;
            }
            /* let this fall through since bufState is partialMsg */
        }
        case partialMsg: {
            /* msgLenBytes is now valid */
            if(validBufferBytes >= msgLenBytes) {
                /* we have a complete message */
                bufState = completeMsg;
                msgCounter++;
                return 0;
            }
            else {
                /* return bytes left in buffer */
                return maxBufferBytes - validBufferBytes; 
            }
            
        }
        case completeMsg: {
            /* should not call this method in completeMsg state.
             * After return of completed message, message should be
             * removed calling this method. */
            return ERR_EXAM_BUF_CALL_OUT_OF_SEQ;
        }
        default: {
            /* only above states valid....should not get here */
            return ERR_ILLEGAL_BUF_STATE;
            
        }
    }
}

int C37_Buffer::removeMsgFromBuffer() {
    /* check proper state */
    if(((bufState != completeMsg) ||
        (msgLenBytes > validBufferBytes)) ||
        (msgLenBytes > maxBufferBytes)) {
        /* not good...bail */
        return ERR_BUF_STATE_MANGLED;
    }
    
    int j = 0;
    for (int i = msgLenBytes; i < validBufferBytes + 1; i++) {
        buffer[j++] = buffer[i];
    }
    validBufferBytes -= msgLenBytes;
    bufState = unknownMsg;
    msgLenBytes = 0;
    /* return remaining length in buffer */
    return maxBufferBytes - validBufferBytes;
}

int C37_Buffer::removeC37MsgFromBuffer(uint8_t * dataBuf) {
    /* check proper state */
    if(((bufState != completeMsg) ||
        (msgLenBytes > validBufferBytes)) ||
        (msgLenBytes > maxBufferBytes)) {
        /* not good...bail */
        return ERR_BUF_STATE_MANGLED;
    }
    
    int j = 0;
    for (int i = msgLenBytes; i < validBufferBytes + 1; i++) {
        dataBuf[j] = buffer[i];
        buffer[j++] = buffer[i];
    }
    validBufferBytes -= msgLenBytes;
    bufState = unknownMsg;
    msgLenBytes = 0;
    /* return remaining length in buffer */
    return maxBufferBytes - validBufferBytes;
}

int C37_Buffer::removeRawMsgFromBuffer(uint8_t * dataBuf) {
    /* check proper state */
    if(((bufState != completeMsg) ||
        (msgLenBytes > validBufferBytes)) ||
        (msgLenBytes > maxBufferBytes)) {
        /* not good...bail */
        return ERR_BUF_STATE_MANGLED;
    }
    
    int j = 0;
    for (int i = msgLenBytes; i < validBufferBytes + 1; i++) {
        dataBuf[j] = buffer[i];
        buffer[j++] = buffer[i];
    }
    validBufferBytes -= msgLenBytes;
    bufState = unknownMsg;
    msgLenBytes = 0;
    /* return remaining length in buffer */
    return maxBufferBytes - validBufferBytes;
}

boost::asio::mutable_buffers_1 C37_Buffer::getBufferDesc() {
    return boost::asio::buffer(&buffer[validBufferBytes], 
        (maxBufferBytes - validBufferBytes)); 
}
//}

