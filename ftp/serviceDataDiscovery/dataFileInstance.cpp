/*
 * Stream-Processing Architecture for Real-time Cyber-physical Security (SPARCS)
 */


/* 
 * File:   dataFileInstance.cpp
 * Author: mcp
 * 
 * Created on January 25, 2016, 4:11 AM
 */

#include <iostream>
#include "dataFileInstance.h"

using namespace std;

namespace serviceDataDiscovery {
    
dataFileInstance::dataFileInstance() {
}

dataFileInstance::~dataFileInstance() {
}

void dataFileInstance::setURL(std::string ipAddr) {
    url = ipAddr;
}

void dataFileInstance::setDawnTimeHoriz(int yr, int mon, int day, int hr) {
    dawnYr = yr;
    currYr = yr;
    dawnMon = mon;
    currMon = mon;
    dawnDay = day;
    currDay = day;
    dawnHr = hr;
    currHr = hr;
}

void dataFileInstance::setSunsetTimeHoriz(int yr, int mon, int day, int hr) {
    sunsetYr = yr;
    sunsetMon = mon;
    sunsetDay = day;
    sunsetHr = hr;
}
    
bool dataFileInstance::incTimeSpec() {
    tempYr = currYr;
    tempMon = currMon;
    tempDay = currDay;
    tempHr = currHr;
    
   
    if(tempHr < 23) {
        tempHr++;
    }
    else {
        tempHr = 0;
        if(tempDay < 32) {
            tempDay++;
        }
        else {
            tempDay = 1;
            if(tempMon < 13) {
                tempMon++;
            }
            else {
                tempMon = 1;
                tempYr++;
            }
        }
    }
    if(isValidTimeSpec()) {
        std::cout <<"yr: " << tempYr  <<  " mo: " << tempMon << " day: "
                << tempDay << " hr: " << tempHr << std::endl;
        currYr = tempYr;
        currMon = tempMon;
        currDay = tempDay;
        currHr = tempHr;
        
        //createTimeSpecString();
        return true;
    }
    else {
        return false;
    }
}

bool dataFileInstance::isValidTimeSpec() {
    if(tempYr > dawnYr) {
        return true;
    } 
    else if (tempYr < dawnYr) {
        return false;
    }
    return true;
}

std::string dataFileInstance::getFileURL() {
    return "url";
}
}