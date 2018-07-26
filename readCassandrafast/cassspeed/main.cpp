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

/*
 * File:   main.cpp
 * Author: rainer
 *
 * Created on December 6, 2016, 4:25 PM
 */

#include <stdio.h>
#include <cassandra.h>
#include <iostream>
#include <string.h>
#include <chrono>
#include "upmuDataProtobuf.pb.h"
#include <vector>
#include <algorithm>    // std::min_element, std::max_element

#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <netinet/in.h>
#include <resolv.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <iostream>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include "cassspeed.pb.h"
#include <string>
#include <sstream>
#include <cstring> //for strcmp
#include <vector>
 #include <bitset>

std::string lookuptable[] = {
    "Min",
    "Max",
    "Avg",
    "Skip",
    "Raw",
};

#define CHECK_BIT(var,pos) ((var) & (1<<(pos)))

void* SocketHandler(void*);

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






#define samplerate 120;

using namespace serviceCommon;

using namespace std;
using namespace google::protobuf::io;


typedef struct Credentials_ {
  const char* username;
  const char* password;
} Credentials;

int getdata(void* lp);
int numsamples=0;

void on_auth_initial(CassAuthenticator* auth,
                       void* data) {
  /*
   * This callback is used to initiate a request to begin an authentication
   * exchange. Required resources can be acquired and initialized here.
   *
   * Resources required for this specific exchange can be stored in the
   * auth->data field and will be available in the subsequent challenge
   * and success phases of the exchange. The cleanup callback should be used to
   * free these resources.
   */

  /*
   * The data parameter contains the credentials passed in when the
   * authentication callbacks were set and is available to all
   * authentication exchanges.
   */
  const Credentials* credentials = (const Credentials *)data;

  size_t username_size = strlen(credentials->username);
  size_t password_size = strlen(credentials->password);
  size_t size = username_size + password_size + 2;

  char* response = cass_authenticator_response(auth, size);

  /* Credentials are prefixed with '\0' */
  response[0] = '\0';
  memcpy(response + 1, credentials->username, username_size);

  response[username_size + 1] = '\0';
  memcpy(response + username_size + 2, credentials->password, password_size);
}

void on_auth_challenge(CassAuthenticator* auth,
                       void* data,
                       const char* token,
                       size_t token_size) {
  /*
   * Not used for plain text authentication, but this is to be used
   * for handling an authentication challenge initiated by the server.
   */
}

void on_auth_success(CassAuthenticator* auth,
                     void* data,
                     const char* token,
                     size_t token_size ) {
  /*
   * Not used for plain text authentication, but this is to be used
   * for handling the success phase of an exchange.
   */
}

void on_auth_cleanup(CassAuthenticator* auth, void* data) {
  /*
   * No resources cleanup is necessary for plain text authentication, but
   * this is used to cleanup resources acquired during the authentication
   * exchange.
   */
}

void compressor_skip_sample(std::vector<float>* input,std::vector<float>* output,unsigned int stepsize){
    for (int i=0; i<input->size()-stepsize;i=i+stepsize){ // changed from for (int i=0; i<input->size();i=i+stepsize){
        output->push_back(input->at(i));
    }
    numsamples=output->size();

}

void compressor_raw(std::vector<float>* input,std::vector<float>* output,unsigned int stepsize){
    *output=*input;
    numsamples=output->size();
}


void compressor_min_sample(std::vector<float>* input,std::vector<float>* output,unsigned int stepsize){

    for (int i=0; i<=input->size()-stepsize;i=i+stepsize){
        float min=input->at(i);
        //std::cout<< "compressor_min_sample(i=" << i << ", stepsize=" << stepsize << ") : " << min << std::endl;
        for(int j=i+1;j<i+stepsize;j++){
            if (input->at(j)<min)
                min=input->at(j);
        }
        output->push_back(min);

    }
    numsamples=output->size();

}

void compressor_max_sample(std::vector<float>* input,std::vector<float>* output,unsigned int stepsize){

    for (int i=0; i<=input->size()-stepsize;i=i+stepsize){
        float max=input->at(i);
        for(int j=i+1;j<i+stepsize;j++){
            if (input->at(j)>max)
                max=input->at(j);
        }
        output->push_back(max);

    }
    numsamples=output->size();

}
void compressor_avg_sample(std::vector<float>* input,std::vector<float>* output,unsigned int stepsize){

    for (int i=0; i<=input->size()-stepsize;i=i+stepsize){
        float avg=input->at(i);
        for(int j=i+1;j<i+stepsize;j++){
                avg=avg+input->at(j);
        }
        avg=avg/stepsize;
        output->push_back(avg);

    }
    numsamples=output->size();

}

void makemysocket(){


        int host_port= 9000;

        struct sockaddr_in my_addr;

        int hsock;
        int * p_int ;
        int err;

        socklen_t addr_size = 0;
        int* csock;
        sockaddr_in sadr;
        pthread_t thread_id=0;

        hsock = socket(AF_INET, SOCK_STREAM, 0);
        if(hsock == -1){
                printf("Error initializing socket %d\n", errno);
                goto FINISH;
        }

        p_int = (int*)malloc(sizeof(int));
        *p_int = 1;

        if( (setsockopt(hsock, SOL_SOCKET, SO_REUSEADDR, (char*)p_int, sizeof(int)) == -1 )||
                (setsockopt(hsock, SOL_SOCKET, SO_KEEPALIVE, (char*)p_int, sizeof(int)) == -1 ) ){
                printf("Error setting options %d\n", errno);
                free(p_int);
                goto FINISH;
        }
        free(p_int);

        my_addr.sin_family = AF_INET ;
        my_addr.sin_port = htons(host_port);

        memset(&(my_addr.sin_zero), 0, 8);
        my_addr.sin_addr.s_addr = INADDR_ANY ;

        if( bind( hsock, (sockaddr*)&my_addr, sizeof(my_addr)) == -1 ){
                fprintf(stderr,"Error binding to socket, make sure nothing else is listening on this port %d\n",errno);
                goto FINISH;
        }
        if(listen( hsock, 10) == -1 ){
                fprintf(stderr, "Error listening %d\n",errno);
                goto FINISH;
        }

        //Now lets do the server stuff

        addr_size = sizeof(sockaddr_in);

        while(true){
                printf("waiting for a connection\n");
                csock = (int*)malloc(sizeof(int));
                if((*csock = accept( hsock, (sockaddr*)&sadr, &addr_size))!= -1){
                        printf("---------------------\nReceived connection from %s\n",inet_ntoa(sadr.sin_addr));
                        pthread_create(&thread_id,0,&SocketHandler, (void*)csock );
                        pthread_detach(thread_id);
                }
                else{
                        fprintf(stderr, "Error accepting %d\n", errno);
                }
        }

FINISH:
;//oops
}

google::protobuf::uint32 readHdr(char *buf)
{
  google::protobuf::uint32 size;
  google::protobuf::io::ArrayInputStream ais(buf,4);
  CodedInputStream coded_input(&ais);
  coded_input.ReadVarint32(&size);//Decode the HDR and get the size
  cout<<"size of payload is "<<size<<endl;
  return size;
}







void readBody(int csock,google::protobuf::uint32 siz)
{
  int bytecount;
  upmuData payload;
  char buffer [siz+4];//size of the payload and hdr
  //Read the entire buffer including the hdr
  if((bytecount = recv(csock, (void *)buffer, 4+siz, MSG_WAITALL))== -1){
                fprintf(stderr, "Error receiving data %d\n", errno);
        }
  cout<<"Second read byte count is "<<bytecount<<endl;
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
  cout<<"Message is "<<payload.DebugString();

}

void* SocketHandler(void* lp){
    int *csock = (int*)lp;


         int bytecount=0;

         char buffer[1024]; //TODO set this right was 4 before
         char oldbuffer[1024];
         while(1){
         memset(buffer, '\0', 1024);
         bytecount=0;
        if (( bytecount = recv(*csock, buffer, 1024, MSG_PEEK))== -1)
        {
                fprintf(stderr, "Error receiving data %d\n", errno);
                close(*csock);
                return 0;
        }
         if (bytecount==0){
             fprintf(stderr, "No data recieved\n");
             free(csock);
             return 0;
         }
//         if((std::strcmp(oldbuffer, buffer) == 0)){
//             std::cout << "nonewdata ";
//             fprintf(stderr, "No new data recieved\n");
//             close(csock);
//             return;
//         }

         std::cout << "received " << bytecount << "bytes"<< std::endl;
         std::cout << "content: " << buffer;
        //else //if (bytecount == 0)
             //   break;

//        string output,pl;
//        upmuData logp;
//
//        memset(buffer, '\0', 4);
//
//        while (1) {
//        //Peek into the socket and get the packet size
//        if((bytecount = recv(*csock,
//                         buffer,
//                                 4, MSG_PEEK))== -1){
//                fprintf(stderr, "Error receiving data %d\n", errno);
//        }else if (bytecount == 0)
//                break;
//        cout<<"First read byte count is "<<bytecount<<endl;
//        readBody(*csock,readHdr(buffer));
//        }
//        getdata(lp)
//
//
//
//
//}
//
//void send_protobuf(CodedOutputStream *coded_output){
//
//}
//
//
//
//
//
//int getdata(void* lp){


   // int *csock=(int*)lp
  // Verify that the version of the library that we linked against is
  // compatible with the version of the headers we compiled against.
  GOOGLE_PROTOBUF_VERIFY_VERSION;

//  std::vector< float > o;
//  std::vector< float > t;
//  t.push_back(1);
//  t.push_back(2);
//  t.push_back(3);
//  t.push_back(2.5);
//  t.push_back(3.5);
//  t.push_back(2.2);
//  t.push_back(1.5);
//  compressor_avg_sample(&t,&o,2);
//  std::copy(o.begin(), o.end(), std::ostream_iterator<float>(std::cout, " "));

   CassAuthenticatorCallbacks auth_callbacks = {
    on_auth_initial,
    on_auth_challenge,
    on_auth_success,
    on_auth_cleanup
  };

  Credentials credentials = {
    "readonly",
    "readonly"
  };


  /* Setup and connect to cluster */
  CassFuture* connect_future = NULL;
  CassCluster* cluster = cass_cluster_new();
  CassSession* session = cass_session_new();
  const char* hosts = "yourserver.com"
  
  std::string errorcode="";
//  if (argc > 1) {
//    hosts = argv[1];
//  }

      std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();

      try{
  /* Add contact points */
  cass_cluster_set_contact_points(cluster, hosts);

    /* Set custom authentication callbacks and credentials */
  cass_cluster_set_authenticator_callbacks(cluster,
                                           &auth_callbacks,
                                           NULL,
                                           &credentials);

  /* Provide the cluster object as configuration to connect the session */
  connect_future = cass_session_connect(session, cluster);
  int i=0;


  //std::vector< uint64_t > timestamp ;
  cass_int64_t timestampcass;
  bool firstsample=true;


  if (cass_future_error_code(connect_future) == CASS_OK) {
    CassFuture* close_future = NULL;

    std::string strbuffer(buffer);
    std::vector<std::string> x = split(strbuffer, ';');





    int i=0;
    int numdevices=std::stoi(x.at(i++));
    std::string *devices = new std::string[numdevices];
    std::cout<<"num devices is" << numdevices <<std::endl;
    int j=0;
    for (;j<numdevices;j++){
        devices[j]=x.at(i);

        i++;
    }
    std::cout<<i << std::endl;
    int nosamplesets=std::stoi(x.at(i++));
//    std::string *samplesets = new std::string[nosamplesets];
//    for (j=0;j<numdevices;j++){
//        samplesets[j]=x.at(i++);
//    }
    std::string timestampstart=x.at(i++);
    std::string timestampend=x.at(i++);
    bool requested[12];
    for (int k=0;k<12;k++){
        requested[k]=std::stoi(x.at(i++))? 1 : 0 ;
    }
    int compression_method= std::stoi(x.at(i++));
    int stepsize= std::stoi(x.at(i++));



    uint64_t tmp;
    std::istringstream stream (timestampstart);
    stream >> tmp;
    int daystart= int((tmp-999)/86400000);//cutoff rounding //-1000 to read earlier
    timestampstart= std::to_string(tmp-999);
    int samples_to_skip=(tmp%1000)/8.33333333;
    std::cout << "samples_to_skip is "<< samples_to_skip << std::endl;
    
    uint64_t tmp2;
    std::istringstream stream2 (timestampend);
    stream2 >> tmp2;
    int dayend= int(tmp2/86400000);//cutoff rounding
    std::string day = std::to_string(daystart);
    for (int i=daystart;i<dayend;i++){
        day.append(","+to_string(i+1));
    }

    std::cout<<"the device is "<< devices[0] << std::endl;




    //std::string devices[]={"test1"};
   // int numdevices=1;
    //std::string samplesets[]={"L1mag","L2mag"};
    //bool requested[]={1,1,0,0,0,0,0,0,0,0,0,0};
    //int compression_method=1;
    //int stepsize = 20000;

    /* Build statement and execute query */
    //const char* query = "SELECT release_version FROM system.local";
    //const char* query = "SELECT * FROM upmu_devel.upmu_data WHERE device =  'P3001199'  AND day = 17133 AND timestamp_msec >= 1480355400000 AND timestamp_msec < 1480356100000 ALLOW FILTERING";

    serviceCassSpeed::cassspeed payload ;
    for (int itter=0; itter< numdevices;itter++){
    //float L1mag[120];
    std::vector< float > L1mag;
    std::vector< float > L2mag;
    std::vector< float > L3mag;
    std::vector< float > L1ang;
    std::vector< float > L2ang;
    std::vector< float > L3ang;
    std::vector< float > C1mag;
    std::vector< float > C2mag;
    std::vector< float > C3mag;
    std::vector< float > C1ang;
    std::vector< float > C2ang;
    std::vector< float > C3ang;




    std::string querystr="SELECT * FROM upmu_devel.upmu_data WHERE device =  '"+devices[itter]+"'  AND day in ("+day+") AND timestamp_msec >= "+timestampstart+" AND timestamp_msec < "+timestampend+" ALLOW FILTERING";

    std::cout<<"the querrystring is" << querystr << std::endl;
    const char* query = querystr.c_str();
    CassStatement* statement = cass_statement_new(query, 0);
    CassFuture* result_future = cass_session_execute(session, statement);

    if (cass_future_error_code(result_future) == CASS_OK) {
      /* Retrieve result set and get the first row */
      const CassResult* result = cass_future_get_result(result_future);
      //const CassRow* row = cass_result_first_row(result);
      /* Create a new row iterator from the result */
      CassIterator* row_iterator = cass_iterator_from_result(result);

    while (cass_iterator_next(row_iterator)) {
        const CassRow* row = cass_iterator_get_row(row_iterator);

        if (row) {
            size_t output_size;
            cass_byte_t* output;


            const CassValue* valuedata = cass_row_get_column_by_name(row, "data");
            const CassValue* valuedatatimestamp = cass_row_get_column_by_name(row, "timestamp_msec");
            cass_int64_t timestampcassnow;

                cass_value_get_int64(valuedatatimestamp,
                     &timestampcassnow);
                if (firstsample){
                     timestampcass=timestampcassnow;
                     firstsample=false;
                 }

                if (timestampcassnow< timestampcass){
                    timestampcass=timestampcassnow;
                }
                
                //std::cout<<"timestamp" << timestampcass << std::endl;
               //
            //}

            cass_value_get_bytes(valuedata,
                        &output,
                       &output_size);
            upmuData unpack;
            unpack.ParseFromArray(output, output_size);

            //std::cout <<unpack.numsamples();
            //uint64_t unpacktimestamp=(uint64_t)unpack.timestamp()*1000;
            //uint64_t interval=(uint64_t)unpack.sampleintervalmsec();
            //std::cout<< unpacktimestamp << std::endl;
            for(int i=0; i<unpack.numsamples();i++){
             auto loop= unpack.sample(i);
             L1mag.push_back(loop.l1mag());
             L2mag.push_back(loop.l2mag());
             L3mag.push_back(loop.l3mag());
             L1ang.push_back(loop.l1angle());
             L2ang.push_back(loop.l2angle());
             L3ang.push_back(loop.l3angle());
             C1mag.push_back(loop.c1mag());
             C2mag.push_back(loop.c2mag());
             C3mag.push_back(loop.c3mag());
             C1ang.push_back(loop.c1angle());
             C2ang.push_back(loop.c2angle());
             C3ang.push_back(loop.c3angle());
             //timestamp.push_back(unpacktimestamp);
             //unpacktimestamp = unpacktimestamp + interval;
             //std::cout<< unpacktimestamp << std::endl;

            }
            //const CassValue* value = cass_row_get_column_by_name(row, "DEVICE");
            //std::cout<< output_size << std::endl;
          //const char* release_version;
          //size_t release_version_length;
          //cass_value_get_string(value, &release_version, &release_version_length);
          //printf("release_version: '%.*s'\n", (int)release_version_length,
          //       release_version);
          i++;
        }

      }
      cass_iterator_free(row_iterator);
      std::cout <<"counter is" << i<< std::endl;
      //std::cout <<"flaot is" << L1mag<< std::endl;
      //std::copy(L1mag.begin(), L1mag.end(), std::ostream_iterator<float>(std::cout, " "));
      //std::copy(timestamp.begin(), timestamp.end(), std::ostream_iterator<uint64_t>(std::cout, " "));
      cass_result_free(result);

      //remove the samples we dont need from the front
      L1mag.erase(L1mag.begin(),L1mag.begin()+samples_to_skip);
      L2mag.erase(L2mag.begin(),L2mag.begin()+samples_to_skip);
      L3mag.erase(L3mag.begin(),L3mag.begin()+samples_to_skip);
      L1ang.erase(L1ang.begin(),L1ang.begin()+samples_to_skip);
      L2ang.erase(L2ang.begin(),L2ang.begin()+samples_to_skip);
      L3ang.erase(L3ang.begin(),L3ang.begin()+samples_to_skip);
      C1mag.erase(C1mag.begin(),C1mag.begin()+samples_to_skip);
      C2mag.erase(C2mag.begin(),C2mag.begin()+samples_to_skip);
      C3mag.erase(C3mag.begin(),C3mag.begin()+samples_to_skip);
      C1ang.erase(C1ang.begin(),C1ang.begin()+samples_to_skip);
      C2ang.erase(C2ang.begin(),C2ang.begin()+samples_to_skip);
      C3ang.erase(C3ang.begin(),C3ang.begin()+samples_to_skip);
      
      std::cout<<L1mag.size();

      //std::cout<<L1mag.data();


//    std::vector< float > output;
//    compressor_min_sample(&L1mag,&output,40000);
//    std::copy(output.begin(), output.end(), std::ostream_iterator<float>(std::cout, " "));
//    std::vector< float > outputm;
//    compressor_max_sample(&L1mag,&outputm,40000);
//    std::copy(outputm.begin(), outputm.end(), std::ostream_iterator<float>(std::cout, " "));
//    std::vector< float > outputa;
//    compressor_avg_sample(&L1mag,&outputa,40000);
//    std::copy(outputa.begin(), outputa.end(), std::ostream_iterator<float>(std::cout, " "));
//
      
      
      
    timestampcass=timestampcass+samples_to_skip*8.33333; //fix for skipped samples

    payload.set_numdevices(numdevices);
    payload.set_timestampstart(timestampcass);


        serviceCassSpeed::output * device = payload.add_outputsets();
        device->set_deviceid(devices[itter]);

        int compression_method_loop;
        for(int compress=1;compress<=5;compress++){
            if (!CHECK_BIT(compression_method,compress-1)){
                continue;
            }
            else
                compression_method_loop=compress;
            std::cout<< "compression_method_loop: " << compression_method_loop << std::endl;

            if (requested[0]){
                serviceCassSpeed::samplesets * sampleset = device->add_sampleset();
                sampleset->set_name("L1mag");
                sampleset->set_compression(lookuptable[compress-1]);


                std::vector< float > temp_output;
                std::cout<< "Compression Stepsize=" << stepsize << std::endl;
                switch(compression_method_loop){
                    case 1: compressor_min_sample(&L1mag,&temp_output,stepsize); break;
                    case 2: compressor_max_sample(&L1mag,&temp_output,stepsize); break;
                    case 3: compressor_avg_sample(&L1mag,&temp_output,stepsize); break;
                    case 4: compressor_skip_sample(&L1mag,&temp_output,stepsize); break;
                    case 5: compressor_raw(&L1mag,&temp_output,stepsize); break;
                }
                for (int l=0; l<temp_output.size();l++){
                   sampleset->add_sample(temp_output.at(l));
                }
            }
           if (requested[1]){
                serviceCassSpeed::samplesets * sampleset = device->add_sampleset();
                sampleset->set_name("L2mag");
                sampleset->set_compression(lookuptable[compress-1]);

                std::vector< float > temp_output;
                switch(compression_method_loop){
                    case 1: compressor_min_sample(&L2mag,&temp_output,stepsize); break;
                    case 2: compressor_max_sample(&L2mag,&temp_output,stepsize); break;
                    case 3: compressor_avg_sample(&L2mag,&temp_output,stepsize); break;
                    case 4: compressor_skip_sample(&L2mag,&temp_output,stepsize); break;
                    case 5: compressor_raw(&L2mag,&temp_output,stepsize); break;
                }
                for (int l=0; l<temp_output.size();l++){
                   sampleset->add_sample(temp_output.at(l));
                }
            }
            if (requested[2]){
                serviceCassSpeed::samplesets * sampleset = device->add_sampleset();
                sampleset->set_name("L3mag");
                sampleset->set_compression(lookuptable[compress-1]);

                std::vector< float > temp_output;
                switch(compression_method_loop){
                    case 1: compressor_min_sample(&L3mag,&temp_output,stepsize); break;
                    case 2: compressor_max_sample(&L3mag,&temp_output,stepsize); break;
                    case 3: compressor_avg_sample(&L3mag,&temp_output,stepsize); break;
                    case 4: compressor_skip_sample(&L3mag,&temp_output,stepsize); break;
                    case 5: compressor_raw(&L3mag,&temp_output,stepsize); break;
                }
                for (int l=0; l<temp_output.size();l++){
                   sampleset->add_sample(temp_output.at(l));
                }
            }
            if (requested[3]){
                serviceCassSpeed::samplesets * sampleset = device->add_sampleset();
                sampleset->set_name("L1ang");
                sampleset->set_compression(lookuptable[compress-1]);

                std::vector< float > temp_output;
                switch(compression_method_loop){
                    case 1: compressor_min_sample(&L1ang,&temp_output,stepsize); break;
                    case 2: compressor_max_sample(&L1ang,&temp_output,stepsize); break;
                    case 3: compressor_avg_sample(&L1ang,&temp_output,stepsize); break;
                    case 4: compressor_skip_sample(&L1ang,&temp_output,stepsize); break;
                    case 5: compressor_raw(&L1ang,&temp_output,stepsize); break;
                }
                for (int l=0; l<temp_output.size();l++){
                   sampleset->add_sample(temp_output.at(l));
                }
            }
           if (requested[4]){
                serviceCassSpeed::samplesets * sampleset = device->add_sampleset();
                sampleset->set_name("L2ang");
                sampleset->set_compression(lookuptable[compress-1]);

                std::vector< float > temp_output;
                switch(compression_method_loop){
                    case 1: compressor_min_sample(&L2ang,&temp_output,stepsize); break;
                    case 2: compressor_max_sample(&L2ang,&temp_output,stepsize); break;
                    case 3: compressor_avg_sample(&L2ang,&temp_output,stepsize); break;
                    case 4: compressor_skip_sample(&L2ang,&temp_output,stepsize); break;
                    case 5: compressor_raw(&L2ang,&temp_output,stepsize); break;
                }
                for (int l=0; l<temp_output.size();l++){
                   sampleset->add_sample(temp_output.at(l));
                }
            }
            if (requested[5]){
                serviceCassSpeed::samplesets * sampleset = device->add_sampleset();
                sampleset->set_name("L3ang");
                sampleset->set_compression(lookuptable[compress-1]);

                std::vector< float > temp_output;
                switch(compression_method_loop){
                    case 1: compressor_min_sample(&L3ang,&temp_output,stepsize); break;
                    case 2: compressor_max_sample(&L3ang,&temp_output,stepsize); break;
                    case 3: compressor_avg_sample(&L3ang,&temp_output,stepsize); break;
                    case 4: compressor_skip_sample(&L3ang,&temp_output,stepsize); break;
                    case 5: compressor_raw(&L3ang,&temp_output,stepsize); break;
                }
                for (int l=0; l<temp_output.size();l++){
                   sampleset->add_sample(temp_output.at(l));
                }
            }
            if (requested[6]){
                serviceCassSpeed::samplesets * sampleset = device->add_sampleset();
                sampleset->set_name("C1mag");
                sampleset->set_compression(lookuptable[compress-1]);

                std::vector< float > temp_output;
                switch(compression_method_loop){
                    case 1: compressor_min_sample(&C1mag,&temp_output,stepsize); break;
                    case 2: compressor_max_sample(&C1mag,&temp_output,stepsize); break;
                    case 3: compressor_avg_sample(&C1mag,&temp_output,stepsize); break;
                    case 4: compressor_skip_sample(&C1mag,&temp_output,stepsize); break;
                    case 5: compressor_raw(&C1mag,&temp_output,stepsize); break;
                }
                for (int l=0; l<temp_output.size();l++){
                   sampleset->add_sample(temp_output.at(l));
                }
            }
           if (requested[7]){
                serviceCassSpeed::samplesets * sampleset = device->add_sampleset();
                sampleset->set_name("C2mag");
                sampleset->set_compression(lookuptable[compress-1]);

                std::vector< float > temp_output;
                switch(compression_method_loop){
                    case 1: compressor_min_sample(&C2mag,&temp_output,stepsize); break;
                    case 2: compressor_max_sample(&C2mag,&temp_output,stepsize); break;
                    case 3: compressor_avg_sample(&C2mag,&temp_output,stepsize); break;
                    case 4: compressor_skip_sample(&C2mag,&temp_output,stepsize); break;
                    case 5: compressor_raw(&C2mag,&temp_output,stepsize); break;
                }
                for (int l=0; l<temp_output.size();l++){
                   sampleset->add_sample(temp_output.at(l));
                }
            }
            if (requested[8]){
                serviceCassSpeed::samplesets * sampleset = device->add_sampleset();
                sampleset->set_name("C3mag");
                sampleset->set_compression(lookuptable[compress-1]);

                std::vector< float > temp_output;
                switch(compression_method_loop){
                    case 1: compressor_min_sample(&C3mag,&temp_output,stepsize); break;
                    case 2: compressor_max_sample(&C3mag,&temp_output,stepsize); break;
                    case 3: compressor_avg_sample(&C3mag,&temp_output,stepsize); break;
                    case 4: compressor_skip_sample(&C3mag,&temp_output,stepsize); break;
                    case 5: compressor_raw(&C3mag,&temp_output,stepsize); break;
                }
                for (int l=0; l<temp_output.size();l++){
                   sampleset->add_sample(temp_output.at(l));
                }
            }
            if (requested[9]){
                serviceCassSpeed::samplesets * sampleset = device->add_sampleset();
                sampleset->set_name("C1ang");
                sampleset->set_compression(lookuptable[compress-1]);

                std::vector< float > temp_output;
                switch(compression_method_loop){
                    case 1: compressor_min_sample(&C1ang,&temp_output,stepsize); break;
                    case 2: compressor_max_sample(&C1ang,&temp_output,stepsize); break;
                    case 3: compressor_avg_sample(&C1ang,&temp_output,stepsize); break;
                    case 4: compressor_skip_sample(&C1ang,&temp_output,stepsize); break;
                    case 5: compressor_raw(&C1ang,&temp_output,stepsize); break;
                }
                for (int l=0; l<temp_output.size();l++){
                   sampleset->add_sample(temp_output.at(l));
                }
            }
           if (requested[10]){
                serviceCassSpeed::samplesets * sampleset = device->add_sampleset();
                sampleset->set_name("C2ang");
                sampleset->set_compression(lookuptable[compress-1]);

                std::vector< float > temp_output;
                switch(compression_method_loop){
                    case 1: compressor_min_sample(&C2ang,&temp_output,stepsize); break;
                    case 2: compressor_max_sample(&C2ang,&temp_output,stepsize); break;
                    case 3: compressor_avg_sample(&C2ang,&temp_output,stepsize); break;
                    case 4: compressor_skip_sample(&C2ang,&temp_output,stepsize); break;
                    case 5: compressor_raw(&C2ang,&temp_output,stepsize); break;
                }
                for (int l=0; l<temp_output.size();l++){
                   sampleset->add_sample(temp_output.at(l));
                }
            }
            if (requested[11]){
                serviceCassSpeed::samplesets * sampleset = device->add_sampleset();
                sampleset->set_name("C3ang");
                sampleset->set_compression(lookuptable[compress-1]);

                std::vector< float > temp_output;
                switch(compression_method_loop){
                    case 1: compressor_min_sample(&C3ang,&temp_output,stepsize); break;
                    case 2: compressor_max_sample(&C3ang,&temp_output,stepsize); break;
                    case 3: compressor_avg_sample(&C3ang,&temp_output,stepsize); break;
                    case 4: compressor_skip_sample(&C3ang,&temp_output,stepsize); break;
                    case 5: compressor_raw(&C3ang,&temp_output,stepsize); break;
                }
                for (int l=0; l<temp_output.size();l++){
                   sampleset->add_sample(temp_output.at(l));
                }
            }

        }
        //std::cout << "numsamples" << numsamples << std::endl;
        //std::cout<<"timediff" << (numsamples*1000*stepsize/120) << std::endl;
        payload.set_timestampend(timestampcass+(numsamples*1000*stepsize/120) );
        payload.set_numsamples(numsamples);





    } else {
      /* Handle error */
      const char* message;
      size_t message_length;
      cass_future_error_message(result_future, &message, &message_length);
      fprintf(stderr, "Unable to run query: '%.*s'\n", (int)message_length,
                                                            message);
      errorcode="-4";
      
 
    }

    cass_statement_free(statement);
    cass_future_free(result_future);


  }
    /* Close the session */
    close_future = cass_session_close(session);
    cass_future_wait(close_future);
    cass_future_free(close_future);
    
    
    if(errorcode=="-4"){
        //then one of the requests did not go through
        if (payload.has_numsamples()){
            //then at least one device succeeded
            std::cout << "at least one device id did not return data do nothing as of Abdelrahmans request" << std::endl;
            
        }
        else{
             cass_future_free(connect_future);
            cass_cluster_free(cluster);
            cass_session_free(session);
            std::string wha= "-4";
            int siz = wha.size()+4;
            char *pkt = new char [siz];
            pkt=wha.c_str();
            send(*csock, (void *) pkt,siz,0);
            close(*csock);

            free(csock) ;
            return 0;
              close(*csock);
              return 0;
            }
    }

    cout<<"size after serializing is "<<payload.ByteSize()<<endl;
    int siz = payload.ByteSize()+4;
    char *pkt = new char [siz];
    google::protobuf::io::ArrayOutputStream aos(pkt,siz);
    CodedOutputStream *coded_output = new CodedOutputStream(&aos);
    coded_output->WriteVarint32(payload.ByteSize());
    payload.SerializeToCodedStream(coded_output);
    int bytecount;
    int bytesleft=siz;
    int bytescum=0;
//    while(bytesleft>0){
//        int sentsize=bytesleft;
//    if (sentsize>1000000-4)
//            sentsize=1000000-4;
//     if( (bytecount=send(*csock, (void *) &pkt[bytescum],sentsize,0))== -1 ) {
//                        fprintf(stderr, "Error sending data %d\n", errno);
//                        close(*csock);
//                        return 0;
//                }
//                printf("Sent bytes %d\n", bytecount);
//                bytesleft=bytesleft-bytecount;
//                std::cout << "so many bytes left " <<bytesleft << endl;
//                bytescum= bytescum+ bytecount;
//                //printf("closing the socket now");
//                //close(*csock);
//    }
    if( (bytecount=send(*csock, (void *) pkt,siz,0))== -1 ) {
                        fprintf(stderr, "Error sending data %d\n", errno);
                        close(*csock);
                        return 0;
                }
                printf("Sent bytes %d\n", bytecount);

  }else {
    /* Handle error */
    const char* message;
    size_t message_length;
    cass_future_error_message(connect_future, &message, &message_length);
    fprintf(stderr, "Unable to connect: '%.*s'\n", (int)message_length,
                                                        message);


    cass_future_free(connect_future);
    cass_cluster_free(cluster);
    cass_session_free(session);
    std::string wha= "-3";
    int siz = wha.size()+4;
    char *pkt = new char [siz];
    pkt=wha.c_str();
    send(*csock, (void *) pkt,siz,0);
    close(*csock);

    free(csock) ;
    return 0;
  }




  cass_future_free(connect_future);
  cass_cluster_free(cluster);
  cass_session_free(session);

  std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>( t2 - t1 ).count();
  std::cout << "seconds execution time: "<< (double)duration/1000000 << "s"<< std::endl;
   strcpy(oldbuffer,buffer);
   free(csock);
   return 0;
}

catch (const std::invalid_argument& ia) {
	  std::cerr << "Invalid argument: " << ia.what() << '\n';
          cass_future_free(connect_future);
          cass_cluster_free(cluster);
          cass_session_free(session);
          std::string wha= "-1";
          int siz = wha.size()+4;
          char *pkt = new char [siz];
          pkt=wha.c_str();
          send(*csock, (void *) pkt,siz,0);
          close(*csock);
          //delete *pkt;
          free(csock);
          return 0;
}
catch( const std::exception& e) {
        std::cerr << "Standard Exception" << std::endl;
        std::cerr << e.what() << std::endl;
        cass_future_free(connect_future);
        cass_cluster_free(cluster);
        cass_session_free(session);
        std::string wha= "-2";
        int siz = wha.size()+4;
        char *pkt = new char [siz];
        pkt=wha.c_str();
        send(*csock, (void *) pkt,siz,0);
        close(*csock);
        //delete pkt;
        free(csock);
        return 0;
}
catch(...){
        std::cerr << "We caught an exception of an undetermined type" << '\n';
        cass_future_free(connect_future);
        cass_cluster_free(cluster);
        cass_session_free(session);
        std::string wha= "-2";
        int siz = wha.size()+4;
        char *pkt = new char [siz];
        pkt=wha.c_str();
        send(*csock, (void *) pkt,siz,0);
        close(*csock);
        //delete pkt;
        free(csock);
        return 0;
    }
         }


  return 0;

  FINISH:
        free(csock);

}



int main(int argc, char* argv[]) {
    makemysocket();
    return 0;
}
