/*
 */

/* 
 * File:   C37_DataChannel.h
 * Author: mcp
 *
 * Created on April 12, 2017, 12:35 AM
 */

#ifndef C37_DATA_CHANNEL_H
#define C37_DATA_CHANNEL_H

#define CIRCULAR_BUFFER_ELEM_SIZE 100
#define CIRCULAR_BUFFER_NUM_ELEM 200

//namespace C37_Util {
    
class C37_DataChannel {
public:
    C37_DataChannel(boost::asio::io_service& IO, 
            boost::asio::ip::tcp::endpoint& ENDPT);
    virtual ~C37_DataChannel();
    void connect();
    void sendCFG_1cmd();
    void sendCFG_2cmd();
    //void sendCFG_3cmd(); //not implemented yet
    //void sendEXTcmd(unsigned char * data, unsigned int dataLen); //not implemented yet
    
    std::atomic<uint64_t> C37eventsRecv{0};
    std::atomic<uint64_t> C37eventsQueued{0};
    std::atomic<uint64_t> C37eventsDropped{0};
    std::atomic<uint64_t> NumConnects{0};
    std::atomic<uint64_t> BytesRecv{0};
    
private:
    void sendCommand(unsigned int commandType, 
        unsigned char * data, unsigned int dataLen);
    void connect_handler(const boost::system::error_code &ec);
    void  read_handler(const boost::system::error_code &ec,
        std::size_t bytes_transferred);
    void send_handler(const boost::system::error_code &ec);
    
    boost::asio::io_service& io;
    boost::asio::ip::tcp::endpoint& endpoint;
    std::shared_ptr<boost::asio::ip::tcp::socket> sock;
   
    C37_Reformat c37reformat;
    C37_Buffer C37_buf;
    C37_CfgDescriptor C37_desc;
    C37_Cfg_Data * cfg_1_Descriptor = nullptr;
    C37_Cfg_Data * cfg_2_Descriptor = nullptr;
    int tempCnt{0};
    
    boost::crc_basic<16> crc_ccitt = boost::crc_basic<16>( 0x1021, 0xFFFF, 0, false, false );
};
//}

#endif /* C37_DATA_CHANNEL_H */

