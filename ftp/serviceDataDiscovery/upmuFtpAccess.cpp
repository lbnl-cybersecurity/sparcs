/*
 * Stream-Processing Architecture for Real-time Cyber-physical Security (SPARCS)
 */


/* 
 * File:   upmuFtpAccess.cpp
 * Author: mcp
 * 
 * Created on February 5, 2016, 3:19 AM
 */

#include <sstream>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <set>
#include <cstring>
#include <curl/curl.h>
#include <boost/tokenizer.hpp>

//#include "serviceDataDiscovery.h"

#include "uPMUgmtime.h"
#include "upmuFtpAccess.h"

#define DIRECTORY_PREFIX "/upmu_data" //for new firmware

//#define DIRECTORY_PREFIX "" //for old firmware <3.4
namespace serviceDataDiscovery {
    
using namespace serviceCommon;
using namespace std;
using namespace boost;
 
upmuFtpAccess::upmuFtpAccess() {
}

upmuFtpAccess::upmuFtpAccess(const upmuFtpAccess& orig) {
}

upmuFtpAccess::~upmuFtpAccess() {
}

size_t upmuFtpAccess::WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userrp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *) userrp;
    mem->memory = (char *)std::realloc(mem->memory, mem->size + realsize + 1);
    if(mem->memory == NULL) {
        printf("not enough memory\n");
        return 0;
    }
    std::memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;
    return realsize;
}
 
size_t upmuFtpAccess::GetInfoHeaderCallback(void *ptr, size_t size, size_t nmemb, void *data)
{
    (void)ptr;
    (void)data;
    /* we are not interested in the headers itself,
     so we only return the size we would have saved ... */ 
    return (size_t)(size * nmemb);
}

size_t upmuFtpAccess::GetInfoDataCallback(void *ptr, size_t size, size_t nmemb, void *data)
{
    (void)ptr;
    (void)data;
    /* we are not interested in the headers itself,
     so we only return the size we would have saved ... */ 
    return (size_t)(size * nmemb);
}

 int upmuFtpAccess::verifyDataDirPresent(std::string s) {
    /* curl specific structs */
    CURL *curl;
    CURLcode res;
   
    int sts;
    curl = curl_easy_init();
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_USERNAME, ftpUsername.c_str());
        curl_easy_setopt(curl, CURLOPT_PASSWORD, ftpPasswd.c_str());
        std::string cmd = ftpAccessString;
        cmd.append(s);
        curl_easy_setopt(curl, CURLOPT_URL, cmd.c_str());
        curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);
        res = curl_easy_perform(curl);
        
        if(res == CURLE_OK) {
            sts = DIR_PRESENT;           
        }
        else if(res == CURLE_OPERATION_TIMEDOUT) {
            sts  = FTP_NOT_RESP;
        }
        else {
            sts = DIR_NOT_PRESENT;
        }
        curl_easy_cleanup(curl);
    }   
    return sts;
}
 
unsigned int upmuFtpAccess::createTimeFromDataFileName(std::string dataFileName) {
    std::string underscore("_");
    std::string dotDat(".dat");
    DATA_DIR tempDD;
    //std::cout << "DataFileName 1: " << dataFileName.c_str() << std::endl;
    std::string::size_type n = dataFileName.find(underscore);
    dataFileName.erase(0, n + 1);
    //std::cout << "DataFileName 2: " << dataFileName.c_str() << std::endl;
    n = dataFileName.find(dotDat);
    dataFileName.erase(n, dataFileName.length() - 1);
    //std::cout << "DataFileName 3: " << dataFileName.c_str() << std::endl;
    /* get a set of tokens */
    char_separator<char> sep("-");
    //std::string list(chunk.memory);
    tokenizer<char_separator<char>> tokens(dataFileName, sep);
    /* manually iterate over tokens */
    tokenizer<char_separator<char>>::iterator tok_iter = tokens.begin();
    
    struct tm t = {0};  // Initalize to all 0's
    t.tm_year = std::stoi(*tok_iter++) - 1900;  // This is year-1900
    t.tm_mon = std::stoi(*tok_iter++) - 1;  // jan == 0
    t.tm_mday = std::stoi(*tok_iter++);
    t.tm_hour = std::stoi(*tok_iter++);
    t.tm_min = std::stoi(*tok_iter++);   //secPtr->sectionTime.min;
    t.tm_sec = std::stoi(*tok_iter++);   //secPtr->sectionTime.sec;
    t.tm_zone = "GMT";
    time_t et = _mkgmtime(&t);
    //std::cout << "DataFileName 4: " << et << std::endl;
    return (unsigned int)et;
}
 
unsigned int upmuFtpAccess::createTimeFromDataDir(DATA_DIR * dataDir) {
    struct tm t = {0};  // Initalize to all 0's
        t.tm_year = dataDir->year - 1900;  // This is year-1900
        t.tm_mon = dataDir->month - 1;  // jan == 0
        t.tm_mday = dataDir->day;
        t.tm_hour = dataDir->hour;
        t.tm_min = 0;   //secPtr->sectionTime.min;
        t.tm_sec = 0;   //secPtr->sectionTime.sec;
        t.tm_zone = "GMT";
        time_t et = _mkgmtime(&t);
        return (unsigned int)et;
}

int upmuFtpAccess::incDataDir(DATA_DIR *dataDir) {
    dataDir->hour++;
    dataDir->hour = dataDir->hour % 24;
    if(dataDir->hour == 0) {
        dataDir->epochDay++;
    }
    time_t et = dataDir->epochDay * 86400;
    struct  tm * t = gmtime(&et);
    dataDir->year = t->tm_year + 1900;
    dataDir->month = t->tm_mon + 1;
    dataDir->day = t->tm_mday;
    dataDir->state = proposed;
    dataDir->dirName = createDirName_YrMonDayHr(dataDir);
    return isDataDirAtPresent(dataDir);
}

int upmuFtpAccess::isDataDirAtPresent(DATA_DIR *dataDir){
    time_t rawtime;
    time(&rawtime);
    struct tm * t = gmtime(&rawtime);
    if((dataDir->year == t->tm_year + 1900) &&
        (dataDir->month == t->tm_mon + 1) &&
        (dataDir->day == t->tm_mday) &&
        (dataDir->hour == t->tm_hour)) {
        return DATA_DIR_AT_PRESENT;
    }
    return DATA_DIR_OK;
}

std::string upmuFtpAccess::createDirName_Yr(DATA_DIR * dd) {
    std::stringstream ss;
    
    ss << DIRECTORY_PREFIX << "/" << std::setw(4) << dd->year << "/";  
    return ss.str();
}

std::string upmuFtpAccess::createDirName_YrMon(DATA_DIR * dd) {
    std::stringstream ss;
    
    ss << DIRECTORY_PREFIX << "/" << std::setw(4) << dd->year
            <<"/Month_" << setw(2) << std::setfill('0') << dd->month << "/";   
    return ss.str();
}

std::string upmuFtpAccess::createDirName_YrMonDay(DATA_DIR * dd) {
    std::stringstream ss;
    
    ss << DIRECTORY_PREFIX<< "/" << std::setw(4) << dd->year
            <<"/Month_" << setw(2) << std::setfill('0') << dd->month 
            << "/Day_" << setw(2) << std::setfill('0')<< dd->day << "/";  
    return ss.str();
}

std::string upmuFtpAccess::createDirName_YrMonDayHr(DATA_DIR * dd) {
    std::stringstream ss;
    
    ss << DIRECTORY_PREFIX << "/" << std::setw(4) << dd->year
            <<"/Month_" << setw(2) << std::setfill('0') << dd->month 
            << "/Day_" << setw(2) << std::setfill('0') << dd->day 
            << "/Hour_" << setw(2) << std::setfill('0') << dd->hour << "/";   
    return ss.str();
}

int upmuFtpAccess::getDataFileList(std::set<std::string> * fileList,
        std::set<std::string> * processedFileList, std::string * dirName) {
    /* curl specific structs */
    CURLcode res;
    struct MemoryStruct chunk;
    chunk.memory = (char *)std::malloc(1000);
    chunk.size = 0;
        
    if(curlDirServices == NULL) {
        curlDirServices = curl_easy_init();
    }
    if(curlDirServices) {   
        curl_easy_setopt(curlDirServices, CURLOPT_USERNAME, ftpUsername.c_str());
        curl_easy_setopt(curlDirServices, CURLOPT_PASSWORD, ftpPasswd.c_str());
        std::string cmd = ftpAccessString;
        cmd.append(*dirName);
        curl_easy_setopt(curlDirServices, CURLOPT_URL, cmd.c_str());
        curl_easy_setopt(curlDirServices, CURLOPT_DIRLISTONLY, 1);
        curl_easy_setopt(curlDirServices, CURLOPT_URL, cmd.c_str());
        curl_easy_setopt(curlDirServices, CURLOPT_WRITEFUNCTION, 
            upmuFtpAccess::ftpWriteMemoryCallback);
        curl_easy_setopt(curlDirServices, CURLOPT_WRITEDATA, (void *)&chunk);
        //curl_easy_setopt(curlDirServices, CURLOPT_USERAGENT, "libcurl-agent/1.0");
        res = curl_easy_perform(curlDirServices);
        
        if(res != CURLE_OK) {
            curl_easy_cleanup(curlDirServices);
            curlDirServices = NULL;
            return DATA_ACCESS_ERROR;
        }
    }
    else {
        /* don't have valid curl channel. */
        return DATA_ACCESS_ERROR;
    }
    /* get a set of file names */
    char_separator<char> sep("\n");
    std::string list(chunk.memory);
    tokenizer<char_separator<char>> tokens(list, sep);

    for (const auto& t : tokens) {
        std::string fileName(t);
        if(processedFileList == NULL) {
            /* always add filename.  There is no processed files set that
             filters out already read files.*/
            fileList->insert(fileName);
            std::cout << "Found new file: " << fileName.c_str() << std::endl;
        }
        else {
            /* if procesedFileList was passed, filter additions that are added.
             Only add new filenames that do not appear in the processed list. */
            if(processedFileList->find(fileName) == processedFileList->end()){
                fileList->insert(fileName);
                std::cout << "Found new file  w/filter: " << fileName.c_str() << std::endl;
            }
        }
    }
    /* free up the receive buffer */
    std::free(chunk.memory);
    return fileList->size();
}


int upmuFtpAccess::getDataFileLength(std::string *dirName, std::string * fileName,
    double *fileLength) {
    /* curl specific structs */
    CURLcode res;
    double fileSize;
   
    if(curlFileInfoServices == NULL) {
        curlFileInfoServices = curl_easy_init();
    }
    if(curlFileInfoServices) {
        struct MemoryStruct chunk;
        chunk.memory = (char *)std::malloc(1000);
        chunk.size = 0; 
    
        curl_easy_setopt(curlFileInfoServices, CURLOPT_USERNAME, ftpUsername.c_str());
        curl_easy_setopt(curlFileInfoServices, CURLOPT_PASSWORD, ftpPasswd.c_str());
        std::string cmd = ftpAccessString;
        cmd.append(*dirName);
        cmd.append(*fileName);
        curl_easy_setopt(curlFileInfoServices, CURLOPT_URL, cmd.c_str());
        curl_easy_setopt(curlFileInfoServices, CURLOPT_NOBODY, 1L);
        curl_easy_setopt(curlFileInfoServices, CURLOPT_HEADER, 0L);
        curl_easy_setopt(curlFileInfoServices, CURLOPT_HEADERFUNCTION, 
            upmuFtpAccess::ftpGetInfoHeaderCallback);
        curl_easy_setopt(curlFileInfoServices, CURLOPT_WRITEFUNCTION, 
            upmuFtpAccess::ftpGetInfoDataCallback);
        //curl_easy_setopt(curlFileInfoServices, CURLOPT_USERAGENT, "libcurl-agent/1.0");
        res = curl_easy_perform(curlFileInfoServices);
        if(res == CURLE_OK) {
            curl_easy_getinfo(curlFileInfoServices, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &fileSize);
            *fileLength = fileSize;
            return DATA_ACCESS_OK;
        }
        curl_easy_cleanup(curlFileInfoServices);
        curlFileInfoServices = NULL;  
    }
    return DATA_ACCESS_ERROR;
}
    
int upmuFtpAccess::getDataFile(std::string * dirName, std::string * fileName,
        unsigned char ** buffer) {
    /* curl specific structs */
    CURLcode res;
    if(curlDataTransServices == NULL) {
        /* open a curl channel and leave it open */
        curlDataTransServices = curl_easy_init();
    }
    if(curlDataTransServices) {
        struct MemoryStruct chunk;
        chunk.memory = (char *)std::malloc(1000);
        chunk.size = 0;
        curl_easy_setopt(curlDataTransServices, CURLOPT_USERNAME, ftpUsername.c_str());
        curl_easy_setopt(curlDataTransServices, CURLOPT_PASSWORD, ftpPasswd.c_str());
        std::string cmd = ftpAccessString;
        cmd.append(*dirName);
        cmd.append(*fileName);
        curl_easy_setopt(curlDataTransServices, CURLOPT_URL, cmd.c_str());
        curl_easy_setopt(curlDataTransServices, CURLOPT_WRITEFUNCTION, 
            upmuFtpAccess::ftpWriteMemoryCallback);
        curl_easy_setopt(curlDataTransServices, CURLOPT_WRITEDATA, (void *)&chunk);
        //curl_easy_setopt(curlDataTransServices, CURLOPT_USERAGENT, "libcurl-agent/1.0");
        res = curl_easy_perform(curlDataTransServices);       
        if(res != CURLE_OK) {
            curl_easy_cleanup(curlDataTransServices);
            curlDataTransServices = NULL;
            return DATA_ACCESS_ERROR;
        }
        *buffer = (unsigned char *)chunk.memory;
        return chunk.size;
    }
}

}

