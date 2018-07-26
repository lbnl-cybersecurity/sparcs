/*
 */

/* 
 * File:   C37_ReFormat.h
 * Author: mcp
 *
 * Created on December 8, 2017, 11:26 PM
 */

#ifndef C37_REFORMAT_H
#define C37_REFORMAT_H


class C37_Reformat {
public:
    C37_Reformat();
    virtual ~C37_Reformat();
    void processC37event(uint32_t time, uint32_t fracTime, C37_Buffer * C37buf);
private:
    void appendToEventBuf();
    void saveC37event(C37_Buffer * C37buf);
    void processEventBuffer();
    void postProcessUPMUdata(uint8_t *dataBuf, unsigned int dataLen);
    
    enum states {syncToSec, fillBuffer};
    states currentState{syncToSec};
    bool syncStarted{false};
    bool sampleCntValid{false};
    uint16_t sampleCnt{0};
    uint16_t currentSampleCnt{0};
    uint32_t previousSec{0};
    uint32_t presentSec{0};
    uint32_t fracSec[0];
    std::array<uint32_t, 512> fracSecValues;
    uint16_t C37eventBufIndex;
    std::vector<uint32_t> C37EventTime; //Reinhard add
    std::vector<std::unique_ptr<uint8_t []>> C37eventBuf;
    std::atomic<uint64_t> nonStdC37events{0};
    std::atomic<uint64_t> syncToSecAborts{0};
    std::atomic<uint64_t> missingC37events{0};
    std::atomic<uint64_t> extendedMissingC37events{0};
    std::atomic<uint64_t> C37eventSequenceFailures{0};
    std::atomic<uint64_t> processedC37_1sec_Buffers{0};

};

#endif /* C37_REFORMAT_H */

