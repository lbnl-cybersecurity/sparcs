#
#
#
# Create tablespace directory on /media/var for use by
# postgres.  Note: this should keep db overruns from
# using up OS diskspace.
sudo mkdir /media/var/lib/postgresql
sudo chown postgres:postgres /media/var/lib/postgresql
sudo chmod 700 /media/var/lib/postgresql
#
# Postgres configurations prior to creating and accessing
# tables.
# 1. set postgres password within postgres applicatiion.
# 2. create uPMUgateway user with password.
# 3. create the uPMUgateway database within postgres app.
# 4. create a tablespace for data in the 
#   /media/var/lib/postgresql directory.  Tables are pointed
#   to this directory when they are created.  See individual
#   postgres table creation scripts.
#
sudo su postgres
psql
ALTER ROLE postgres WITH ENCRIPTED PASSWORD 'YOURPASSWORD';
CREATE USER uPMUgateway WITH PASSWORD 'YOURPASSWORD';
ALTER ROLE uPMUgateway WITH SUPERUSER;
CREATE DATABASE upmugateway OWNER uPMUgateway;
CREATE TABLESPACE upmu_datastore LOCATION '/media/var/lib/postgresql';
\q
exit
#
# allow remote connections using postgres accounts and passwords
# Note: change outward facing network below - see (**** change as needed ****)
sudo su postgres
CONF=/etc/postgresql/9.4/main/postgresql.conf
sed -i "s/#listen_addresses = 'localhost'/listen_addresses = '*'/" $CONF
#
# configure postgres to accept remote connections
cat >> /etc/postgresql/9.4/main/pg_hba.conf <<EOF
# Accept IPv4 connections (**** change as needed ****)
host    all         all         192.168.11.0/24             trust
EOF
#
exit
#
sudo service postgresql restart
#
# run postgres command files to create initial tables
#
cd /media/home/ceds/uPMU/postgres
sudo su postgres
psql -f uPMU_service.sql
psql -f uPMU_operations.sql
psql -f uPMU_data_directory.sql
psql -f uPMU_data_file.sql
psql -f uPMU_cassandra.sql
psql -f uPMU_ftp_comm.sql
exit
#

