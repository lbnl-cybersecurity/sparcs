
# SPARCS
This software reads from PSL micro-PMUs (µPMUs) and sends data out to RabbitMQ and Cassandra. There are two versions of this code --- one that reads via the C37.118 protocol and the other grabs data via FTP. The RabbitMQ data is formatted in JSON format and can easily be ingested in other systems. We are using an ELK stack and also have sucessfully pushed data to Splunk and OSISoft-Pisystem. Cassandra data is encoded with Protocol Buffers (protobufs) and can be read via the readfastfromCassandra tool that quickly extracts data out of cassandra, and optionaly and ships it to a remote client (included in package). This tool is best run close to the Cassandra server for low latency / packet loss.

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
