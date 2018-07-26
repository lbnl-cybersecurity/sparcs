/*
 */

/* 
 * File:   upmuFtpAccess.h
 * Author: mcp
 *
 * Created on February 5, 2016, 3:19 AM
 */

#ifndef UPMUFTPACCESS_H
#define UPMUFTPACCESS_H

#define INVALID_DATE -1
#define DIR_PRESENT 0
#define DIR_NOT_PRESENT 1
#define FTP_NOT_RESP -1
#define DATA_DIR_AT_PRESENT 1
#define DATA_DIR_OK 0
#define DATA_DIR_ERROR -2
#define NO_DATA_DIR -1
#define DB_ACCESS_ERROR -1
#define DATA_ACCESS_OK 0
#define DATA_ACCESS_ERROR -1

namespace serviceCommon {
    extern std::string ftpAccessString;
    extern std::string ftpUsername;
    extern std::string ftpPasswd;
    extern std::string ftpIpAddress;
    extern CURL * curlDirServices;
    extern CURL * curlDataTransServices;
    extern CURL * curlFileInfoServices;
    extern CURL * curlHttpServices;
    
}

namespace serviceDataDiscovery {
 
enum DATA_DIR_STATE {current, proposed, abandoned, processed, unknown};

typedef struct DataDirectory {
    unsigned int year;
    unsigned int month;
    unsigned int day;
    unsigned int hour;
    std::string dirName;
    unsigned int epochDay;
    unsigned int dbID;
    DATA_DIR_STATE state;
} DATA_DIR;

/* write to memory struct and callback */
struct MemoryStruct {
    char * memory;
    size_t size;
};

class upmuFtpAccess {
public:
    upmuFtpAccess();
    
    upmuFtpAccess(const upmuFtpAccess& orig);
    virtual ~upmuFtpAccess();
    
    static int verifyDataDirPresent(std::string);
    static int getDataFileList(std::set<std::string> *, std::set<std::string> *,
        std::string *);
    static int getDataFile(std::string *, std::string *, unsigned char **);
    static int getDataFileLength(std::string *, std::string *, double *);
    static int incDataDir(DATA_DIR *);
    static int isDataDirAtPresent(DATA_DIR *);
    static std::string createDirName_Yr(DATA_DIR *);
    static std::string createDirName_YrMon(DATA_DIR *);
    static std::string createDirName_YrMonDay(DATA_DIR *);
    static std::string createDirName_YrMonDayHr(DATA_DIR *);
    static unsigned int createTimeFromDataFileName(std::string);
    static unsigned int createTimeFromDataDir(DATA_DIR *);
    
    
    
private:
    size_t WriteMemoryCallback(void *contents, 
        size_t size, size_t nmemb, void *userrp);
    size_t GetInfoHeaderCallback(void *ptr, size_t size, 
        size_t nmemb, void *data);
    size_t GetInfoDataCallback(void *ptr, size_t size, 
        size_t nmemb, void *data);
    
    /* these are wrappers that use C linkage for callbacks */
    static size_t ftpWriteMemoryCallback(void *buffer, size_t sz, size_t n, void *f) {
        // Call non-static member function.
        static_cast<upmuFtpAccess*>(f)->WriteMemoryCallback(buffer, sz, n, f);
    }
    static size_t ftpGetInfoHeaderCallback(void *buffer, size_t sz, size_t n, void *f) {
        // Call non-static member function.
        static_cast<upmuFtpAccess*>(f)->GetInfoHeaderCallback(buffer, sz, n, f);
    }
    static size_t ftpGetInfoDataCallback(void *buffer, size_t sz, size_t n, void *f) {
        // Call non-static member function.
        static_cast<upmuFtpAccess*>(f)->GetInfoDataCallback(buffer, sz, n, f);
    }
    
    int verifyDataDirPresent();
};

}

#endif /* UPMUFTPACCESS_H */

