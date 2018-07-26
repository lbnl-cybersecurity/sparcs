/*
 *** Copyright Notice ***
Stream-Processing Architecture for Real-time Cyber-physical Security (SPARCS) Copyright (c) 2018, The Regents of the University of California, through Lawrence Berkeley National Laboratory (subject to receipt of any required approvals from the U.S. Dept. of Energy. All rights reserved.
If you have questions about your rights to use or distribute this software, please contact Berkeley Lab's Intellectual Property Office at  IPO@lbl.gov.
NOTICE.  This Software was developed under funding from the U.S. Department of Energy and the U.S. Government consequently retains certain rights. As such, the U.S. Government has been granted for itself and others acting on its behalf a paid-up, nonexclusive, irrevocable, worldwide license in the Software to reproduce, distribute copies to the public, prepare derivative works, and perform publicly and display publicly, and to permit other to do so. 
****************************
*** License Agreement ***
Stream-Processing Architecture for Real-time Cyber-physical Security (SPARCS) Copyright (c) 2018, The Regents of the University of California, through Lawrence Berkeley National Laboratory (subject to receipt of any required approvals from the U.S. Dept. of Energy).  All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
(1) Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
(2) Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
(3) Neither the name of the University of California, Lawrence Berkeley National Laboratory, U.S. Dept. of Energy, nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
You are under no obligation whatsoever to provide any bug fixes, patches, or upgrades to the features, functionality or performance of the source code ("Enhancements") to anyone; however, if you choose to make your Enhancements available either publicly, or directly to Lawrence Berkeley National Laboratory, without imposing a separate written license agreement for such Enhancements, then you hereby grant the following license: a  non-exclusive, royalty-free perpetual license to install, use, modify, prepare derivative works, incorporate into other computer software, distribute, and sublicense such enhancements or derivative works thereof, in binary and source code form.
---------------------------------------------------------------

 */
 
/*9
 * File:   main.cpp
 * Author: rainer
 *
 * Created on December 14, 2016, 11:28 AM
 */
 
#include <cstdlib>
 #include <bitset>
#include <unistd.h>
#include <cmath> //for pow
//#include "upmuDataProtobuf.pb.h"
#include <iostream>
#include <google/protobuf/message.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#include <sys/socket.h>
#include <errno.h>
#include <stdio.h>
#include <netinet/in.h>
#include <resolv.h>
#include <arpa/inet.h>
#include "cassspeed.pb.h"
#include <algorithm>
#include <stdexcept>
#include <fstream>
#include <map>
#include <string>
#include <stdlib.h>
#include <limits>
#include "json/json.h"
#include <stdlib.h>     /* strtoul */
//#include </home/absho/workspace/power_data/libraries/jsoncpp/include/json/json.h>
//#include </home/absho/workspace/power_data/libraries/jsoncpp/include/json/writer.h>

#include <netdb.h> //for hostname
#include <arpa/inet.h> //for hostname


using namespace google::protobuf;
using namespace google::protobuf::io;
using namespace std;
using namespace serviceCassSpeed;
 
typedef map<string, string>::iterator map_iter;

bool quiet = 0; //surpress outputs

std::string lookuptable[] = {
    "Min",
    "Max",
    "Avg",
    "Skip",
    "Raw",
};

void split(const std::string &s, char delim, std::vector<std::string> &elems) {
    std::stringstream ss;
    ss.str(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
}


std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, elems);
    return elems;
}


google::protobuf::uint32 readHdr(char *buf)
{
  google::protobuf::uint32 size;
  google::protobuf::io::ArrayInputStream ais(buf,4);
  CodedInputStream coded_input(&ais);
  coded_input.ReadVarint32(&size);//Decode the HDR and get the size
  if(!quiet){
  cout<<"size of payload is MB "<<float(size)/1000000<<endl;
  }
  return size;
}
 
std::string string_to_hex(const std::string& input)
{
    static const char* const lut = "0123456789ABCDEF";
    size_t len = input.length();
 
    std::string output;
    output.reserve(2 * len);
    for (size_t i = 0; i < len; ++i)
    {
        const unsigned char c = input[i];
        output.push_back(lut[c >> 4]);
        output.push_back(lut[c & 15]);
    }
    return output;
}
 
 
 
long delta_t_chucksize(std::string nodeviceids, std::string nosamplesets, std::string compression_method, std::string stepsize, int targetMB){
    int local = stoi(compression_method);
     int comprerssions = (int) std::bitset<8>(local).count();
    //std::cout <<"calc compression no" << comprerssions << std::endl;
    long bps=120*4*atoi(nodeviceids.c_str())*atoi(nosamplesets.c_str())/atoi(stepsize.c_str())*comprerssions; //bytes per second
    long delta_t=targetMB*pow(2,20)/bps; 
    if(delta_t<=0) delta_t=std::numeric_limits<int>::max()/2000;
    if(!quiet){
    std::cout << "reported max delta t is " << delta_t << "seconds" << std::endl;
    }
    return delta_t;
}
 
 
serviceCassSpeed::cassspeed readBody(int csock,google::protobuf::uint32 siz)
{
    if(!quiet){
    cout << "Reading message" << endl;
    cout << "size reported is" << siz << endl;
    }
    int bytecount;
    serviceCassSpeed::cassspeed payload;
   
    //char buffer [siz+4];//size of the payload and hdr
    //Read the entire buffer including the hdr
    //if((bytecount = recv(csock, (void *)buffer, 4+siz, MSG_WAITALL))== -1){
    
    
   // siz=siz*10;
    
    unsigned char buf[1000000];  //10Kb fixed-size buffer
 unsigned char* buffer  = new unsigned char[siz+4];  //temporary buffer

unsigned char* temp_buf = buf;
unsigned char* end_buf = buf + sizeof(buf);
int iByteCount;
long iByteCountcum=0;

// while(iByteCountcum<(long)siz)
//{       
//    //iByteCount = recv(csock, buffer,(siz+4),0);  
//    iByteCount = recv(csock, buf,(1000000-4),0);   
//    
//    
//    if ( iByteCount > 0 )
//    {
//        //make sure we're not about to go over the end of the buffer
//        if (!((temp_buf + iByteCount) <= end_buf))
//            break;
//
//        fprintf(stderr, "Bytes received: %d\n",iByteCount);
//        memcpy(temp_buf, &buffer[iByteCountcum], iByteCount);
//        memset(&temp_buf[0], 0, sizeof(temp_buf));
//        iByteCountcum= iByteCountcum+ iByteCount;
//    cout << "ibyte counter" << iByteCount << " cumulative" << iByteCountcum << endl ;
//        //temp_buf += iByteCount;
//    }
//    else if ( iByteCount == 0 )
//    {
//        if(temp_buf != buf)
//        {
//            //do process with received data
//        }
//        else
//        {
//            fprintf(stderr, "receive failed");
//            break;
//        }
//    }
//    else
//    {
//        fprintf(stderr, "recv failed: ");
//        break;
//    }
//}  //check for end of buffer
//    
//
//cout << "final ibyte counter" << iByteCount << " cumulative" << temp_buf << endl ;
//    
    
    
    if((bytecount = recv(csock, (void *)buffer, 4+siz, MSG_WAITALL))== -1){
                  fprintf(stderr, "Error receiving data %d\n", errno);
          }
    
    
    
    
    
    
//    if((bytecount = recv(csock, (void *)buffer, 40+siz, MSG_WAITALL))== -1){
//                  fprintf(stderr, "Error receiving data %d\n", errno);
//          }
if(!quiet){
    cout<<"Second read byte count is "<<bytecount<<endl;
}
    //Assign ArrayInputStream with enough memory
    google::protobuf::io::ArrayInputStream ais(buffer,siz+4);
    CodedInputStream coded_input(&ais);
    //Read an unsigned integer with Varint encoding, truncating to 32 bits.
    coded_input.ReadVarint32(&siz);
    //After the message's length is read, PushLimit() is used to prevent the CodedInputStream
    //from reading beyond that length.Limits are used when parsing length-delimited
    //embedded messages
    google::protobuf::io::CodedInputStream::Limit msgLimit = coded_input.PushLimit(siz);
    //De-Serialize
    payload.ParseFromCodedStream(&coded_input);
    //Once the embedded message has been parsed, PopLimit() is called to undo the limit
    coded_input.PopLimit(msgLimit);
    //Print the message
 
    //const Descriptor *desc       = payload.GetDescriptor();
    //const Reflection *refl       = payload.GetReflection();  
    //
    if(!quiet){
    cout<< "The timestamp is " << payload.timestampstart() << std::endl;
    cout<< "The timestampend is " << payload.timestampend() << std::endl;
    cout<< "Number of Samples: " << payload.numsamples() << std::endl;
    cout << "The device id is " << payload.outputsets(0).deviceid() << std::endl;
    }
    //cout << "The device id is " << payload.outputsets(1).deviceid() << std::endl;
    //cout << "The First SampleSet id is " << payload.outputsets(0).sampleset(0).name() << std::endl;
    //cout << "OutputSet Size is " << payload.outputsets_size() << endl;
    //cout << "Descriptor: " << desc->full_name().c_str()
    //        << ", # of fields: " << desc->field_count() << endl;
    //cout << "Value of " << refl->GetUInt64(payload, desc->field(1)) << endl;
   
    return payload;
}

void saveBodyCSV(serviceCassSpeed::cassspeed payload, string filename, float step_size,std::string compression_method)
{
    if(!quiet){
    cout << "Saving message to csv format"  << endl;
    }
    float time_step = (step_size / 120) * 1000;
    int output_len = payload.outputsets_size();
    
    ofstream saved_file_csv;
    saved_file_csv.open (filename + ".csv");
    int local = stoi(compression_method);
    std::bitset<8> foo(local);
    //CSV header
    std::string headeradd="";
    
    for (int i=0;i<5;i++){
        if(foo.test(i)){
            headeradd = headeradd+","+lookuptable[i];
        }
    }
    saved_file_csv << "Device ID" << "," << "TimeStamp" << "," << "Data Type"<< headeradd << "\n";
    for (int i = 0; i < output_len; i++) {
        string deviceID = payload.outputsets(i).deviceid();
        int sampleset_len = payload.outputsets(i).sampleset_size();
        int samples_len = payload.outputsets(i).sampleset(0).sample_size();
        if(!quiet){
        cout << " =============  " << deviceID << endl;
        }
        uint64 timestamp_start = payload.timestampstart();
       
        for (int j = samples_len - 1; j >= 0; j--) {
            map<string, string> data_keys;
           
            for (int k = 0; k < sampleset_len; k++) {
                string data_type_name = payload.outputsets(i).sampleset(k).name();
                string compression_type = payload.outputsets(i).sampleset(k).compression();
                
                int samples_len_check = payload.outputsets(i).sampleset(k).sample_size();
                
                if (j < samples_len_check) {
                    float sample_value = payload.outputsets(i).sampleset(k).sample(j);
                    
                    data_keys[data_type_name] += to_string(sample_value) + ",";
                }
            }
           
            for (map_iter it = data_keys.begin(); it != data_keys.end(); it++) {
                saved_file_csv << deviceID << "," << timestamp_start << ","
                        << it->first << "," << it->second << "\n";
            }
            timestamp_start += (uint64) time_step;
        }
    }
   
    saved_file_csv.close();
}

void saveBodyJson(serviceCassSpeed::cassspeed payload, string filename, float step_size, bool console)
{
    if(!quiet){
    cout << "Saving message to json format"  << endl;
    }
    float time_step = (step_size / 120) * 1000;
    int output_len = payload.outputsets_size();

    
    Json::Value message_json;
    
    ofstream saved_file_json;
    if (!console) {
    saved_file_json.open (filename + ".json");
    }
    for (int i = 0; i < output_len; i++) {
        string deviceID = payload.outputsets(i).deviceid();
        int sampleset_len = payload.outputsets(i).sampleset_size();
        int samples_len = payload.outputsets(i).sampleset(0).sample_size();
        if(!console && !quiet){
        cout << " =============  " << deviceID << endl;
        }
        uint64 timestamp_start = payload.timestampstart();
       
        for (int j = samples_len - 1; j >= 0; j--) {
            map<string, string> data_keys;
           
            for (int k = 0; k < sampleset_len; k++) {
                string data_type_name = payload.outputsets(i).sampleset(k).name();
                string compression_type = payload.outputsets(i).sampleset(k).compression();
                
                int samples_len_check = payload.outputsets(i).sampleset(k).sample_size();
                
                if (j < samples_len_check) {
                    float sample_value = payload.outputsets(i).sampleset(k).sample(j);
                    
                    message_json[deviceID][to_string(timestamp_start)][data_type_name][compression_type] = sample_value;
                    
                    data_keys[data_type_name] += to_string(sample_value) + ",";
                }
            }
            
            timestamp_start += (uint64) time_step;
        }
    }
    if (console) {
    cout << message_json << std::endl;
    }
    else{
    saved_file_json << message_json;
    
    //cout << "Event: " << event << endl;
    saved_file_json.close();
    }
}

std::string HostToIp(const std::string& host) {
    hostent* hostname = gethostbyname(host.c_str());
    if(hostname)
        return std::string(inet_ntoa(**(in_addr**)hostname->h_addr_list));
    return {};
}

bool isValidIpAddress(const char *ipAddress)
{
    struct sockaddr_in sa;
    int result = inet_pton(AF_INET, ipAddress, &(sa.sin_addr));
    return result != 0;
}




int main(int argc, char** argv) {
    
    if (argc < 3) {
        if (!(argc == 2 && (string(argv[1]) == "--help"))) {
            std::cerr << "See: " << argv[0] << " --help" << std::endl;
            return -1;
        }
    }
    
    std::string timestampstart; //msec
    std::string timestampend;
    std::string nodeviceids; //number of devices below
    std::string deviceids;
    std::string nosamplesets;
    std::string requested;  //L1mag,L2mag,L3mag,L1ang,L2ang,L3ang,C1mag,C2mag,C3mag,C1ang,C2ang,C3ang
    std::string compression_method; //1= min; 2=max;4=avg;8=skip_samples ; if you want multiple returns add the numbers together (e.g min+max=3)
    std::string stepsize; // every x sample returned
    std::string calc_delta_t= "0";
    
    
//    std::string timestampstart="1480355400000"; //msec
//    std::string timestampend="1480357100000";
//    std::string nodeviceids="2"; //number of devices below
//    std::string deviceids="P3001199;P3001035";
//    std::string nosamplesets="2";
//    std::string requested="1;1;1;1;1;1;1;1;1;1;1;1";  //L1mag,L2mag,L3mag,L1ang,L2ang,L3ang,C1mag,C2mag,C3mag,C1ang,C2ang,C3ang
//    std::string compression_method="15"; //1= min; 2=max;4=avg;8=skip_samples ; if you want multiple returns add the numbers together (e.g min+max=3)
//    std::string stepsize ="1"; // every x sample returned
    
    
    
    
    
    
    std::string filename ="example";
    std::string format ="csv";
     int host_port= 9000;
     
    std::string host="1.1.1.1";
    host=HostToIp("youserver.com");
    //host="127.0.0.1";
     
    
    for (int i = 1; i < argc; ++i) {
        if (i < argc) {
             if (string(argv[i]) == "--help") {
                 std::cerr << "Usage: " << argv[0] << " --timestart TimeStampStart msec since epoch (req)" << std::endl
                                          << " --timeend TimeStampEnd msec since epoch (req)" << std::endl
                                          << " --host host name/ip of CassReSampleServer (optional)"<< std::endl
                                          << " --port port of CassReSampleServer (optional)"<< std::endl
                                          << " --deviceIds DeviceList (P...) (req) " << std::endl
                                          << " --datacode ReqDataTypeCode (e.g 1;0;0;0;0;0;0;0;0;0;0;0 for L1mag;L2mag;L3mag;L1ang;L2ang;L3ang;C1mag;C2mag;C3mag;C1ang;C2ang;C3ang)(req)" << std::endl
                                          << " --stepsize Sampling_Compression [enter 1 if you want every sample] (req)" << std::endl
                                          << " --compress CompressionFactor (req if stepsize>1) (1= min; 2=max;4=avg;8=skip_samples,16=raw ; multiple combinable)" << std::endl
                                          << " --filename FileName for JSON or CSV (default example) "<< std::endl
                                          << " --format Format [JSON/CSV/console] (only one)(default csv), if you use console --quiet 1 is recommended "<< std::endl
                                          << " --quiet 1 surpress all output but errors (default 0)"<< std::endl
                                          << " --delta_t 1 returns calculated delta_t (default 0)"<< std::endl
                                          << std::endl;
            return 1;
        }
        }
        if (i + 1 < argc) {
            if (string(argv[i]) == "--timestart") {
                // Make sure we aren't at the end of argv!
                timestampstart = argv[++i]; // Increment 'i' so we don't get the argument as the next argv[i].
                //cout<< timestampstart <<endl;
            } else if (string(argv[i]) == "--quiet") {
                quiet = atoi(argv[++i]); 
            }else if (string(argv[i]) == "--timeend") {
                timestampend = argv[++i];
            }else if (string(argv[i]) == "--host") {
                host = argv[++i];
                if(!isValidIpAddress(host.c_str())){
                     host=HostToIp(host);
                }
                
            }else if (string(argv[i]) == "--port") {
                host_port = atoi(argv[++i]);
            } else if (string(argv[i]) == "--deviceNum") {
                std::cout<<"devicenumber no longer needed. Ignoring" << std::endl;
                ++i;
            } else if (string(argv[i]) == "--deviceIds") {
                deviceids = argv[++i];
            } else if (string(argv[i]) == "--datacode") {
                requested = argv[++i];
            } else if (string(argv[i]) == "--stepsize") {
                stepsize = argv[++i];
            } else if (string(argv[i]) == "--delta_t") {
            calc_delta_t = argv[++i];
            } else if (string(argv[i]) == "--compress") {
                compression_method = argv[++i];
            } else if (string(argv[i]) == "--filename") {
                filename = argv[++i];
            } else if (string(argv[i]) == "--format") {
                format = argv[++i];
            } else {
                std::cerr << "Unknown option: " << argv[i] << std::endl;
                return 2;
            }
        } else {
            std::cerr << "--option requires one argument." << std::endl;
            return 2;
        }
    }
    
    //Rudimentary Check input for errors 
    if (stepsize=="1" && compression_method.empty() ){
        compression_method="16";
    }
    try{
     int local = stoi(compression_method);
    std::bitset<8> foo(local);
    if (foo.test(5) && stepsize!="1"){
        std::cerr<<" If raw is requested the stepsize must be 1" << std::endl;
        return 5;
        
    }}
    catch(...){
        std::cerr<< "compression method not set" << std::endl;
        return 6;
    }
   
    
    std::string strbuffer(deviceids);
    std::vector<std::string> x = split(strbuffer, ';');
    nodeviceids=std::to_string(x.size());
    
    strbuffer =requested;
    x = split(strbuffer, ';');
    try{
        int loopsum=0;
        int looptmp;
    for (int loopcounter=0; loopcounter<12; loopcounter++){
        
        looptmp= atoi(x.at(loopcounter).c_str());
        if (!(looptmp!=0 | looptmp!=1)){
            std::cerr<<" only bool datacodes allowed";
            return 3;
        }
        loopsum=loopsum+looptmp;
    }
        
    nosamplesets=std::to_string(loopsum);
    }
    catch(...){
         std::cerr<<" only bool datacodes allowed that are split with \";\". Check that you specified all 12 of them";
         return 4;
    }
    
    long delta_t=delta_t_chucksize( nodeviceids, nosamplesets,compression_method, stepsize,32)*1000;
    if (delta_t<0){
        cerr << "error in delta_t calculation; report to Reinhard" << endl;
        return 65;
    }
    
    if( stoi(calc_delta_t)){
        std::cout << delta_t << std::endl;
        return 0;
    }
    
    //start the loop here
    long timestampstart_global= stol(timestampstart);
    long timestampend_global= stol(timestampend);
    long timestampend_loop =timestampend_global;
    long timestampstart_loop =timestampstart_global;
    for(int loopvar=0; ;loopvar++){
        
        if (timestampstart_loop+delta_t > timestampend_global){
            timestampend_loop=timestampend_global;
        }
        else{
            timestampend_loop=timestampstart_loop+delta_t;
        }
    
    serviceCassSpeed::cassspeed payload ;

    //payload.set_sampleintervalmsec(123);
    //
    //cout<<"size after serilizing is "<<payload.ByteSize()<<endl;
    //int siz = payload.ByteSize()+4;
    //char *pkt = new char [siz];
    //google::protobuf::io::ArrayOutputStream aos(pkt,siz);
    //CodedOutputStream *coded_output = new CodedOutputStream(&aos);
    //coded_output->WriteVarint32(payload.ByteSize());
    //payload.SerializeToCodedStream(coded_output);

    
    //const char* host_name="127.0.0.1";
    struct sockaddr_in my_addr;
    const char* host_name=host.c_str();
    //char buffer[1024];
    int bytecount=0;
    int buffer_len=0;

    int hsock;
    int * p_int;
    int err;

    hsock = socket(AF_INET, SOCK_STREAM, 0);
    if(hsock == -1){
            printf("Error initializing socket with errorno %d\n",errno);
            close(hsock);
            return 0;
            //goto FINISH;
    }

    p_int = (int*)malloc(sizeof(int));
    *p_int = 1;

    if( (setsockopt(hsock, SOL_SOCKET, SO_REUSEADDR, (char*)p_int, sizeof(int)) == -1 )||
            (setsockopt(hsock, SOL_SOCKET, SO_KEEPALIVE, (char*)p_int, sizeof(int)) == -1 ) ){
            printf("Error setting options %d\n",errno);
            free(p_int);
            close(hsock);
            return 0;
            //goto FINISH;
    }
    free(p_int);

    my_addr.sin_family = AF_INET ;
    my_addr.sin_port = htons(host_port);

    memset(&(my_addr.sin_zero), 0, 8);
    my_addr.sin_addr.s_addr = inet_addr(host_name);
    //printf("XXX hostname: %s\n", host_name);

    if( connect( hsock, (struct sockaddr*)&my_addr, sizeof(my_addr)) == -1 ){
            if((err = errno) != EINPROGRESS){
                    fprintf(stderr, "Error connecting socket with errorno  %d\n", errno);
                    close(hsock);
                    return 0;
                    //goto FINISH;
            }
    }

    
    std::string datatotransmit=nodeviceids+";"+deviceids+";"+nosamplesets+";"+std::to_string(timestampstart_loop)+";"+std::to_string(timestampend_loop)+";"+requested+";"+compression_method+";"+stepsize;
    if (!quiet){
    cout << "Data to transmit: " << datatotransmit << endl;
    }
    char* buffer1 = new char[datatotransmit.length()+1];
    strcpy(buffer1,datatotransmit.c_str());

    if( (bytecount=send(hsock, buffer1,datatotransmit.length()+1,0))== -1 ) {

                    fprintf(stderr, "Error sending data %d\n", errno);
                    close(hsock);
                    return 0;
            }
            if (!quiet){
            printf("Sent bytes %d\n", bytecount);
            printf("Sent command, now switching to receiving the response \n");
            }

    char buffer[1024];
    //int bytecount=0;
    string output,pl;
    //upmuData logp;

    //memset(buffer, '\0', 4);

   // while (1) {
    //Peek into the socket and get the packet size
    if((bytecount = recv(hsock,
                     buffer,
                             4, MSG_PEEK))== -1){
            fprintf(stderr, "Transmission already finished or Errorno %d\n", errno);
            close(hsock);
            return 0;
    }else if (bytecount == 0){
            //break;
    }
    if (!quiet){
    cout<<"First read byte count is "<<bytecount<<endl;
    cout<<"Code sent is "<<buffer<<endl;
    }
    if (strcmp(buffer,"-1")==0){ //i have no idea why but -1 became -2 and it works. 
        cerr << "invalid argument in request" <<endl;
        close(hsock);
        return 101;
    }
    if (strcmp(buffer,"-2")==0){
        cerr << "unknown error occurred. Check the request syntax and ensure that you are requesting less than 2GB of data. Keep also in mind that you and the server need to keep all data in memory" << endl;
        close(hsock);
        return 102;
    }
        if (strcmp(buffer,"-3")==0){
        cerr << "CassReSample Server could not connect to Cassandra" << endl;
        close(hsock);
        return 103;
    }
        if (strcmp(buffer,"-4")==0){
        cerr << "Cassandra Error: Unable to run query. Either Request timeout concider changing cassandra.yaml OR least one device id specified is not found in Cassandra  for every day in the time range specified" << endl;
        close(hsock);
        return 104;
    }

    serviceCassSpeed::cassspeed payload_result = readBody(hsock,readHdr(buffer));
    
    std::string filename_loop= filename+"_"+std::to_string(loopvar);
    if (format == "console") {
        saveBodyJson(payload_result, filename_loop, strtof(stepsize.c_str(), NULL),1);
    }
    else if (format == "csv") {
        saveBodyCSV(payload_result, filename_loop, strtof(stepsize.c_str(), NULL),compression_method);
        std::cout<< filename_loop+".csv" << std::endl;
        //std::cout<< "ready: " << filename_loop+".csv" << std::endl;
    } else if (format == "json") {
        saveBodyJson(payload_result, filename_loop, strtof(stepsize.c_str(), NULL),0);
        std::cout<< filename_loop+".json"<< std::endl;
        //std::cout<< "ready: " << filename_loop+".json"<< std::endl;
    } else {
        cerr << "unknown format entered" << endl;
        return -3;
    }
    
//    if (payload_result.has_timestampend()){
//        unsigned long te = strtoul(timestampend.c_str(),NULL,0);
//        std::cout <<te << "equal" <<timestampend << "result " << payload_result.timestampend() << endl;
////        if(floor((timestampend-payload_result.timestampend())/stepsize*120/1000)!=0){
////            
////        }
//    }
//
//    else {
//        std::cerr<<"ERROR: Not end time recieved continueing anyway and report packet done" << std::endl;
//    }
           
        
        
   // }
        
        
//       for (int i =0;i<10000;i++){
//          for (int j = 0 ;j<10;j++) {
//
//                if( (bytecount=send(hsock, (void *) pkt,siz,0))== -1 ) {
//                        fprintf(stderr, "Error sending data %d\n", errno);
//                        goto FINISH;
//                }
//                printf("Sent bytes %d\n", bytecount);
//                usleep(1);
//         }
//        }
//        delete pkt;

//FINISH:
close(hsock);
//cout<<"Message is "<<payload_result.DebugString() << endl;
timestampstart_loop=payload_result.timestampend(); //update the new start to the old end.
if (timestampstart_loop+(stoi(stepsize)+1)*1000/120> timestampend_global){ // RAINER +1 in order to fix rounding errors
    if(!quiet){
        std::cout<< "finished" <<std::endl;
       
    }
    break;
}

    } //closes the timeloop
return 0;

}


//1485247750422
//1485249488012
// ./cassspeedclient --timestart "1480355400000" --timeend "1480356100000" --deviceIds "P3001199;P3001035" --datacode "1;1;0;0;0;0;0;0;0;0;0;0" --stepsize 2000 --compress 15 --filename "example1" --format "csv" --quiet 0

// ./cassspeedclient --timestart "1480355400000" --timeend "1480356100000" --deviceNum 2 --deviceIds "P3001199;P3001035" --datacode "1;1;0;0;0;0;0;0;0;0;0;0" --stepsize 2000 --compress 15 --filename "example1" --format "csv"

// ./cassspeedclient --timestart "1485158400000" --timeend "1485159000000" --deviceNum 1 --deviceIds "P3001035" --datacode "1;1;0;0;0;0;0;0;0;0;0;0" --stepsize 2000 --compress 15 --filename "example2" --format "json"
