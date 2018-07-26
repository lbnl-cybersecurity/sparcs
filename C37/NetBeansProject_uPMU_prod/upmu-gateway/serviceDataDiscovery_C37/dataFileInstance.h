/*
 */

/* 
 * File:   dataFileInstance.h
 * Author: mcp
 *
 * Created on January 25, 2016, 4:11 AM
 */

#ifndef DATAFILEINSTANCE_H
#define DATAFILEINSTANCE_H

namespace serviceDataDiscovery {

class dataFileInstance {
public:
    void setURL(std::string ipAddr);
    void setDawnTimeHoriz(int yr, int mon, int day, int hr);
    void setSunsetTimeHoriz(int yr, int mon, int day, int hr);
    bool incTimeSpec();
    std::string getFileURL();
    
    dataFileInstance();
    virtual ~dataFileInstance();
private:
    std::string url;
    std::string dawnTimeHoriz;
    std::string sunsetTimeHoriz;
    int dawnYr, dawnMon, dawnDay, dawnHr;
    int sunsetYr, sunsetMon, sunsetDay, sunsetHr;
    int currYr, currMon, currDay, currHr;
    int tempYr, tempMon, tempDay, tempHr;
    bool isValidTimeSpec();

};
}
#endif /* DATAFILEINSTANCE_H */

