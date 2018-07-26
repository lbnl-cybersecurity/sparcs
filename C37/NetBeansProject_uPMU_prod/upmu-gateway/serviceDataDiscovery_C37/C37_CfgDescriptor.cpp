/*
 * Stream-Processing Architecture for Real-time Cyber-physical Security (SPARCS)
 */


/* 
 * File:   C37_CfgDescriptor.cpp
 * Author: mcp
 * 
 * Created on April 29, 2017, 6:54 PM
 */
#include <string>
#include <vector>
#include <boost/algorithm/string/trim.hpp>
#include <boost/crc.hpp>      // for boost::crc_basic, boost::crc_optimal
#include <boost/cstdint.hpp>  // for boost::uint16_t

#include "C37_MsgFormats.h"
#include "C37_CfgDescriptor.h"

//namespace C37_Util {


C37_CfgDescriptor::C37_CfgDescriptor() {
    
}

C37_CfgDescriptor::~C37_CfgDescriptor() {
}

C37_Cfg_Data::C37_Cfg_Data() {
    
}

C37_Cfg_Data::~C37_Cfg_Data() {
    
}

/* Conf. header processing: not implemented yet
C37_Cfg_Header_Descriptor::C37_Cfg_Header_Descriptor(unsigned char * buf) :
    C37_CfgDescriptor(buf){
    this->data = std::string ((char *)&buf[DATA_1_INDX], this->frameLength - DATA_1_INDX);    
}

C37_Cfg_Header_Descriptor::~C37_Cfg_Header_Descriptor() {
}
*/

/* Conf. format 1 processing */
C37_Cfg_Data * C37_CfgDescriptor::parseCfg_msg(unsigned char * buf,
        unsigned int msgLen) {
    if((buf[SYNC_INDX] != 0xaa) || 
            ((buf[SYNC_INDX + 1] != CFG_1_MSG_BYTE_VALUE) &&
            (buf[SYNC_INDX + 1] != CFG_2_MSG_BYTE_VALUE))) {
        return nullptr;
    }
    if(msgLen < CFG_1_2_MIN_MSG_LEN) {
        return nullptr;
    }
    crc_ccitt.reset(); 
    crc_ccitt.process_bytes(buf, msgLen - 2);
    unsigned int msgCksum = (buf[msgLen - 2] << 8) + buf[msgLen -1];
    if(crc_ccitt.checksum() != msgCksum) {
        return nullptr;
    }
    C37_Cfg_Data * desc = new C37_Cfg_Data();
    /* Process conf. format 1 info */
    desc->frameType = (buf[SYNC_INDX + 1] & 0x70) >> 4;
    desc->frameLength = buf[FRAMESIZE_INDX + 1] + 
                        (buf[FRAMESIZE_INDX] << 8);
    desc->C37_version = buf[SYNC_INDX + 1] & 0xF;
    desc->dataStreamID = (buf[IDCODE_INDX] << 8) + buf[IDCODE_INDX + 1];
    desc->timeStamp = (buf[SOC_INDX] << 24) + (buf[SOC_INDX + 1] << 16) +
            (buf[SOC_INDX + 2] >>8) + buf[SOC_INDX + 3];
    desc->timeQuality = buf[FRACSEC_INDX];
    desc->rawFracSec = (buf[FRACSEC_INDX + 1] << 16) + 
            (buf[FRACSEC_INDX + 2] << 8) + buf[FRACSEC_INDX + 3];
    desc->uSec_FracSec = 0;
    desc->timeBase = (buf[TIME_BASE_INDX] << 24) + (buf[TIME_BASE_INDX + 1] << 16) +
            (buf[TIME_BASE_INDX + 2] << 8) + buf[TIME_BASE_INDX + 3];
    
    desc->numPMU = (buf[NUM_PMU_INDX] << 8) + buf[NUM_PMU_INDX+ 1];
    /* get number of PMUs contained in data msg */
    std::vector<C37_PMUmetadata> pmuMetadata;
    /* loop through all PMUs and format metadata */
    int baseIndx = FIRST_PMU_INDX;
    for(int pmuIndxi = 0; pmuIndxi < desc->numPMU; pmuIndxi++) {       
        C37_PMUmetadata * md = new C37_PMUmetadata;
        /* search for null or unprintable chars so we can construct a proper
         c++ string object */
        int charCounter = 0;
        for(charCounter = 0; charCounter < 16; charCounter++) {
            if(!isprint(buf[baseIndx + STN_OFFSET + charCounter])) {
                break;
            }
        }
        md->stnName = std::string((const char *)&(buf[baseIndx + STN_OFFSET]), 
                charCounter); 
        
        /* collect format info for data msg */
        if((buf[baseIndx + FORMAT_OFFSET + 1] & 0x8) != 0) {
            md->freqDfreqFloating = true;
        }
        if((buf[baseIndx + FORMAT_OFFSET + 1] & 0x4) != 0) {
            md->analogDataFloating = true;
        }
        if((buf[baseIndx + FORMAT_OFFSET + 1] & 0x2) != 0) {
            md->phasorDataFloating = true;
        }
        if((buf[baseIndx + FORMAT_OFFSET + 1] & 0x1) != 0) {
            md->phasorDataPolar = true;
        }
        
        md->numPhasors = (buf[baseIndx + PHNMR_OFFSET] << 8) + 
                buf[baseIndx + PHNMR_OFFSET + 1];
        md->numAnalog = (buf[baseIndx + ANNMR_OFFSET] << 8) + 
                buf[baseIndx + ANNMR_OFFSET + 1];
        md->numDigital = (buf[baseIndx + DGNMR_OFFSET] << 8) + 
                buf[baseIndx + DGNMR_OFFSET + 1];
        
        /* compute various indicies for this PMU */
        unsigned int chnamIndx = baseIndx + CHNAM_OFFSET;
        unsigned int phasorCFIndx = chnamIndx + 16 * (md->numPhasors + md->numAnalog +
                16 * md->numDigital);
        unsigned int analogCFIndx = phasorCFIndx + 4 * md->numPhasors;
        unsigned int digitalCFIndx = analogCFIndx + 4 * md->numAnalog;
        unsigned int fnomIndx = digitalCFIndx + 4 * md->numDigital;
        unsigned int cfgCntIndx = fnomIndx + 2;
        
        /* get phasor channel names */
        std::vector<C37_phasorChanName> (md->phasorChanName);
        for(int pmuChanIndx = 0; pmuChanIndx < md->numPhasors; pmuChanIndx++) {
            C37_phasorChanName * pnam = new C37_phasorChanName;
            for(charCounter = 0; charCounter < 16; charCounter++) {
                if(!isprint(buf[chnamIndx + charCounter])) {
                    break;
                }
            }
        
            std::string s = std::string((const char *)&(buf[chnamIndx]), 
                charCounter);
            pnam->chanName = s;
            md->phasorChanName.push_back(*pnam);
            chnamIndx += 16;
        }
        
        /* get analog channel names */
        std::vector<C37_analogChanName> (md->analogChanName);
        for(int pmuAnalogIndx = 0; pmuAnalogIndx < md->numAnalog; pmuAnalogIndx++) {
            C37_analogChanName * anam = new C37_analogChanName;
            for(charCounter = 0; charCounter < 16; charCounter++) {
                if(!isprint(buf[chnamIndx + charCounter])) {
                    break;
                }
            }
        
            std::string s = std::string((const char *)&(buf[chnamIndx]), 
                charCounter);
            anam->chanName = s;
            md->analogChanName.push_back(*anam);
            chnamIndx += 16;
        }
        
        /* get digital channel names */
        std::vector<C37_digitalChanName> (md->digitalChanName);
        for(int pmuDigitalIndx = 0; pmuDigitalIndx < md->numDigital; pmuDigitalIndx++) {
            C37_digitalChanName * dnam = new C37_digitalChanName;
            std::vector<std::string> (dnam->digitalBitName);
            for(int bitIndx = 0; bitIndx < 16; bitIndx++) {
                for(charCounter = 0; charCounter < 16; charCounter++) {
                    if(!isprint(buf[chnamIndx + charCounter])) {
                        break;
                    }
                }

                std::string s = std::string((const char *)&(buf[chnamIndx]), 
                    charCounter);
                dnam->digitalBitName.push_back(s);
                chnamIndx += 16;
            }
            md->digitalChanName.push_back(*dnam);           
        }
        
        std::vector<C37_phasorConvFact> (md->phasorCF);
        for(int tempIndx = 0; tempIndx < md->numPhasors; tempIndx++) {
            C37_phasorConvFact * cf = new C37_phasorConvFact;
            if(buf[phasorCFIndx] == 0) {
                cf->isVoltage = true;
            }
            else {
                cf->isVoltage = false;
            }
            cf->convFact = (buf[phasorCFIndx + 1] << 16) + 
                    (buf[phasorCFIndx + 2] << 8) + buf[phasorCFIndx + 3];                    
            md->phasorCF.push_back(*cf);
        }
       
        std::vector<C37_analogConvFact> (md->analogCF);
        for(int tempIndx = 0; tempIndx < md->numAnalog; tempIndx++) {
            C37_analogConvFact * cf = new C37_analogConvFact;
            //set isVoltage
            //set convfact
            md->analogCF.push_back(*cf);
        }
        
        std::vector<C37_digitalConvFact> (md->digitalCF);
        for(int tempIndx = 0; tempIndx < md->numDigital; tempIndx++) {
            C37_digitalConvFact * cf = new C37_digitalConvFact;
            md->digitalCF.push_back(*cf);
        }
        
        if((buf[fnomIndx + 1] & 0x1) == 0) {
            md->fundFreq = 60;
        }
        else {
            md->fundFreq = 50;
        }
        md->cfgCnt = (buf[cfgCntIndx] << 8) + buf[cfgCntIndx +1];
        /* add metadata to list */
        pmuMetadata.push_back(*md);
        /* compute beginning of next element */
        baseIndx = cfgCntIndx + 2;
    }
    
    desc->dataRate = (buf[baseIndx] << 8) + buf[baseIndx];
    return desc;
}

//}

    