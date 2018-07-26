/*
 */

/* 
 * File:   C37_CfgDescriptor.h
 * Author: mcp
 *
 * Created on April 29, 2017, 6:54 PM
 */

#ifndef C37_CFGDESCRIPTOR_H
#define C37_CFGDESCRIPTOR_H

//namespace C37_Util {
  
class C37_phasorChanName {
public:
    std::string chanName;
};

class C37_analogChanName {
public:
    std::string chanName;
};

class C37_digitalChanName {
public:
    std::vector<std::string> digitalBitName;
};

class C37_phasorConvFact {
public:
    bool isVoltage;
    unsigned int convFact;
};

class C37_analogConvFact {
public:
    unsigned int measType;
    unsigned int convFact;
};

class C37_digitalConvFact {
public:
    unsigned int mask0;
    unsigned int mask1;
};

class C37_PMUmetadata {
public:
    std::string stnName;
    unsigned int idCode;
    bool freqDfreqFloating;
    bool analogDataFloating;
    bool phasorDataFloating;
    bool phasorDataPolar;
    unsigned int numPhasors;
    unsigned int numAnalog;
    unsigned int numDigital;
    
    std::vector<C37_phasorChanName> phasorChanName;
    std::vector<C37_analogChanName> analogChanName;
    std::vector<C37_digitalChanName> digitalChanName;
    std::vector<C37_phasorConvFact> phasorCF;
    std::vector<C37_analogConvFact> analogCF;
    std::vector<C37_digitalConvFact> digitalCF;
    unsigned int fundFreq;
    unsigned int cfgCnt;
    
private:
};
    
class C37_Cfg_Data {
public:
    C37_Cfg_Data();
    virtual ~C37_Cfg_Data();
    /* data members */
    unsigned int frameType;
    unsigned int frameLength;
    unsigned int C37_version;
    unsigned int dataStreamID;
    unsigned int timeStamp;
    unsigned int rawFracSec;
    unsigned int uSec_FracSec;
    unsigned int timeQuality;
    unsigned int timeBase;
    unsigned int numPMU;
    std::vector<C37_PMUmetadata> pmuMetadata;
    unsigned int dataRate;
};


class C37_CfgDescriptor {
public:
    C37_CfgDescriptor();
    virtual ~C37_CfgDescriptor();
    C37_Cfg_Data * parseCfg_msg(unsigned char * buf,
        unsigned int msgLen);
private:
    boost::crc_basic<16> crc_ccitt = boost::crc_basic<16>( 0x1021, 0xFFFF, 0, false, false );
};
//}

#endif /* C37_CFGDESCRIPTOR_H */

