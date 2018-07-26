/*
 * Stream-Processing Architecture for Real-time Cyber-physical Security (SPARCS)
 */


/* 
 * File:   C37_DataChannel.cpp
 * Author: mcp
 * 
 * Created on April 12, 2017, 12:35 AM
 */
#define BOOST_BIND_NO_PLACEHOLDERS //prevent error on docker


#include <time.h>
#include <cstdlib>
#include <vector>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/async_result.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/crc.hpp>      // for boost::crc_basic, boost::crc_optimal
#include <boost/cstdint.hpp>  // for boost::uint16_t
#include <boost/circular_buffer.hpp>
#include <boost/circular_buffer/base.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>

/* Json library */
//#include <jsoncpp/json/json.h>

using namespace boost::asio;
using namespace boost::asio::ip;
using namespace boost;

#include "C37_inputDesc.h"
#include "C37_MsgFormats.h"
#include "C37_Buffer.h"
#include "C37_CfgDescriptor.h"
#include "C37_Reformat.h"
#include "C37_DataChannel.h"
    
C37_DataChannel::C37_DataChannel (io_service& IO, 
        boost::asio::ip::tcp::endpoint& ENDPT) : io(IO),
        sock(std::make_shared<boost::asio::ip::tcp::socket>(io)),
        endpoint(ENDPT) {
    //sock = std::make_shared<boost::asio::ip::tcp::socket>(io);
}

C37_DataChannel::~C37_DataChannel() {
}

void C37_DataChannel::connect() {
    sock->async_connect(endpoint, 
        boost::bind(&C37_DataChannel::connect_handler, this,
        boost::asio::placeholders::error)); 
}

/* not implemented yet 
void C37_DataChannel::sendHDRcmd() {
    sendCommand(HDR_CMD_VALUE, nullptr, 0);
}
*/

void C37_DataChannel::sendCFG_1cmd() {
    sendCommand(CFG_1_CMD_VALUE, nullptr, 0);
}

void C37_DataChannel::sendCFG_2cmd() {
    sendCommand(CFG_2_CMD_VALUE, nullptr, 0);
}

/* not implemented yet
void C37_DataChannel::sendCFG_3cmd() {
    sendCommand(CFG_3_CMD_VALUE, nullptr, 0);
}
/
 */

/* not implemented yet
void C37_DataChannel::sendEXTcmd(unsigned char * data, unsigned int dataLen) {
    sendCommand(EXTFR_CMD_VALUE, nullptr, 0);
}
*/

void C37_DataChannel::connect_handler(const boost::system::error_code &ec)
{
  std::cout<< "Connect status: " << ec.message() << std::endl;
  if (!ec)
  {
      NumConnects++;
      boost::asio::async_read(*sock, C37_buf.getBufferDesc(),
          boost::bind(&C37_DataChannel::read_handler, this,
           placeholders::error, placeholders::bytes_transferred));
  }
  else {
      sock->close();
      sock.reset(new boost::asio::ip::tcp::socket(io));
      connect();
  }
}

void C37_DataChannel::read_handler(const boost::system::error_code &ec,
  std::size_t bytes_transferred)
{
    if(bytes_transferred!=0) //Changed by Reinhard
  //if (!ec)
  {
    C37_buf.setBytesReceived(bytes_transferred);
    while (C37_buf.examineBuffer() == 0) {
        BytesRecv += C37_buf.msgLenBytes;
        switch (C37_buf.msgType) {
            case DATA_MSG_TYPE_VALUE: {
                uint32_t fracTime = (C37_buf.buffer[FRACSEC_INDX + 1] << 16) + 
                    (C37_buf.buffer[FRACSEC_INDX + 2] << 8) + C37_buf.buffer[FRACSEC_INDX + 3];
                uint32_t time = (C37_buf.buffer[SOC_INDX] << 24) + (C37_buf.buffer[SOC_INDX + 1] << 16) +
                    (C37_buf.buffer[SOC_INDX + 2] << 8) + C37_buf.buffer[SOC_INDX + 3]; //Reinhard: this is the same as an endianswap
                C37eventsRecv++;
                c37reformat.processC37event(fracTime, time, &C37_buf);
                C37eventsQueued++;
                C37_buf.removeMsgFromBuffer();
                break;
            }
            case CFG_1_MSG_TYPE_VALUE: {
                this->cfg_1_Descriptor = 
                        C37_desc.parseCfg_msg(C37_buf.buffer,
                        C37_buf.msgLenBytes);
                C37_buf.removeMsgFromBuffer();
                break;
            }
            case CFG_2_MSG_TYPE_VALUE: {
                this->cfg_2_Descriptor = 
                        C37_desc.parseCfg_msg(C37_buf.buffer,
                        C37_buf.msgLenBytes);
                C37_buf.removeMsgFromBuffer();
                break;
            }
            default: {
                C37_buf.removeMsgFromBuffer();
                break;
            }
        }     
    }
    
    boost::asio::async_read(*sock, C37_buf.getBufferDesc(),
          boost::bind(&C37_DataChannel::read_handler, this,
           placeholders::error, placeholders::bytes_transferred));
  }
  else {
//      if (ec.message()=="End of file"){
//           boost::asio::
//          boost::asio::async_read(*sock, C37_buf.getBufferDesc(),
//          boost::bind(&C37_DataChannel::read_handler, this,
//           placeholders::error, placeholders::bytes_transferred));
//          std::cout<< "END of file error with " << bytes_transferred <<"bytes_transferred" << std::endl;
//          
//      }
//      else{
        C37_buf.init();
        sock->close();
        std::cout<< "DISCONNECTED reason: "<< ec.message() << std::endl;
        sock.reset(new boost::asio::ip::tcp::socket(io));
        connect();
     // }
  }
}

void C37_DataChannel::sendCommand(unsigned int commandType, 
        unsigned char * data, unsigned int dataLen)
{
    printf("sending command. \n");
    if(data == nullptr) {
        dataLen = 0;
    }
    
    auto sendData = std::make_shared<std::vector<unsigned char> >(EXTFRAME_INDX + dataLen + 2);
    unsigned char x = (*sendData).size();
    printf("Send buffer sizeL %u \n", x);
     
    (*sendData)[SYNC_INDX] = SYNC_VALUE;
    (*sendData)[SYNC_INDX + 1] = CMD_SYNC_VALUE;
    unsigned int frameLen = EXTFRAME_INDX + dataLen + 2;
    (*sendData)[FRAMESIZE_INDX] = (frameLen >> 8) & 0xff;
    (*sendData)[FRAMESIZE_INDX + 1] = frameLen & 0xff;
    (*sendData)[IDCODE_INDX] = 0;
    (*sendData)[IDCODE_INDX + 1] = 1;
    
    std::time_t t = std::time(nullptr);
    
    (*sendData)[SOC_INDX] = (t >> 24) & 0xff;
    (*sendData)[SOC_INDX + 1] = (t >> 16) & 0xff;
    (*sendData)[SOC_INDX + 2] = (t >> 8) & 0xff;
    (*sendData)[SOC_INDX + 3] = t & 0xff;
    (*sendData)[FRACSEC_INDX] = 0;
    (*sendData)[FRACSEC_INDX + 1] = 0;
    (*sendData)[FRACSEC_INDX + 2] = 0;
    (*sendData)[FRACSEC_INDX + 3] = 0;
    (*sendData)[CMD_INDX] = 0;
    (*sendData)[CMD_INDX + 1] = commandType & 0xff;
    this->crc_ccitt.reset(); 
    this->crc_ccitt.process_bytes(&((*sendData)[0]), (*sendData).size());
    //printf("%x \n", crc_ccitt.checksum());
    (*sendData)[dataLen + CMD_INDX + 2] = (crc_ccitt.checksum() >> 8) & 0xff;
    ((*sendData)[dataLen + CMD_INDX + 3]) = crc_ccitt.checksum() & 0xff;
    for(int i = 0; i < (*sendData).size(); ++i) {
        printf("%x\n ", (*sendData)[i]);
    }
    std::cout << std::endl;
    
    boost::asio::async_write(*sock, asio::buffer(*sendData),
        boost::bind(&C37_DataChannel::send_handler, this,
         placeholders::error));
}

void C37_DataChannel::send_handler(const boost::system::error_code &ec)
{
  std::cout<< "Send status: " << ec.message() << std::endl;
  if (!ec)
  {
      //boost::asio::async_read(sock, C37_buf.getBufferDesc(),
          //boost::bind(&C37_DataChannel::read_handler, this,
           //placeholders::error, placeholders::bytes_transferred));
  }
}

//}


