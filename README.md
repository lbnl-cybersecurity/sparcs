
# SPACS
This software reads from uPMUs and sends data out to RabbitMQ and Cassandra. There are 2 versions of this code one that Reads C37.118 and the other grabs data via FTP. The RabbitMQdata is formatted in JSON format and can easily be ingested in other systems. We are using an ELK stack and also have sucessfully pushed data to Splunk and OSISoft-Pisystem. Cassandra data is encoded with protobufs and can be read via the readfastfromCassandra tool that quickly extracts data out of cassandra, and optionaly and ships it to a remote client (included in package). This tool is best run close to the cassandra server for maximum speed.

Configuation files are in the configuration file folder and need to be configured accordingly.



# Install
```
Install instructions for Centos

yum install cmake
yum install gcc
yum install autoconf
yum install openssl-devel
yum install boost
yum install boost-devel
yum install gcc-c++

git clone https://github.com/alanxz/rabbitmq-c
cd rabbitmq-c
mkdir build
cd build
cmake ..
make
make install

cd ..
cd ..

git clone https://github.com/alanxz/SimpleAmqpClient
cd SimpleAmqpClient
mkdir build
cd build
cmake ..
make
make install

cd ..
cd ..


yum install libuv-devel
yum install clang

git clone https://github.com/datastax/cpp-driver.git
mkdir cpp-driver/build
cd cpp-driver/build
cmake ..
b=$(uname -a| grep arm)
[ -n "$b" ] && sed -i 's/ -1 / 255 /g' ../src/uuids.cpp && make -Wno-narrowing
sudo make install

#BBB
[ -n "$b" ] && echo "/usr/local/lib/arm-linux-gnueabihf/" >> /etc/ld.so.conf.d/usr_local.conf
#Debian
[ -z "$b" ] && echo "/usr/local/lib/x86_64-linux-gnu/" >> /etc/ld.so.conf.d/usr_local.conf
sudo ldconfig

yum install prostgesql
yum install prostgesql-server postgresql-contrib
sudo postgresql-setup initdb
vi /var/lib/pgsql/data/pg_hba.conf #change all "ident" or "peer" to "trust" #we might wanna change this at some point. But access is restricted to the localhost anyway.
sudo systemctl start postgresql
sudo systemctl enable postgresql
mkdir /var/lib/postgresql
chown postgres:postgres /var/lib/postgresql/
sudo su postgres
psql
ALTER ROLE postgres WITH ENCRIPTED PASSWORD 'YOURPASSWORD';
CREATE USER uPMUgateway WITH PASSWORD 'YOURPASSWORD';
ALTER ROLE uPMUgateway WITH SUPERUSER;
CREATE DATABASE upmugateway OWNER uPMUgateway;
CREATE TABLESPACE upmu_datastore LOCATION '/var/lib/postgresql';
\q
exit


yum install libpqxx
yum install libpqxx-devel
yum install curl-devel
git clone https://github.com/lbnl-cybersecurity
mkdir /usr/include/boost/dll
cp /upmuGatewayInfo/dll/* /usr/include/boost/dll
yum install bzip2
yum install wget
wget https://sourceforge.net/projects/boost/files/boost/1.61.0/boost_1_61_0.tar.bz2/download
tar -xjfv download
cd boost
sh bootstrap
./b2 install
cd ..
yum install log4cpp

wget https://sourceforge.net/projects/log4cpp/files/log4cpp-1.1.x%20%28new%29/log4cpp-1.1/log4cpp-1.1.1.tar.gz/download
tar -xvzf download
cd log4cpp
./configure
make
make check
make install
cd ..

yum install libtool unzip
wget https://github.com/google/protobuf/releases/download/v2.6.1/protobuf-2.6.1.tar.bz2
tar -jxvf protobuf-2.6.1.tar.bz2
cd protobuf-2.6.1

./autogen.sh
./configure
make
make check
sudo make install
cd ..

echo "/usr/local/lib64" >> /etc/ld.so.conf 
echo "/usr/local/lib" >> /etc/ld.so.conf 
sudo ldconfig # refresh shared library cache.

edit the cassandrauser and cassandrapassword in the main.cpp file in the uPMUgateway directory to your credentials

#change serviceDataDiscovery.h : Set starting month year and day to a day that exists in the upmu e.g today.
#change upmuFtpAccess.cpp #define DIRECTORY_PREFIX "/upmu_data" //for new firmware or #define DIRECTORY_PREFIX "" //for old firmware
#make all the projects
#set postgres files add add them all with psql -f "filename"
mkdir /var/log/upmugateway
chmod 777 /var/log/upmugateway
cp uPMUgateway/upmugateway-devel /etc/init.d/usr_local



#check /etc/init.d/upmugateway-devel and then start it :)

```

# Legal

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


