\connect upmugateway
SET default_tablespace=upmu_datastore;
DROP TABLE IF EXISTS ftp_comm;
DROP TYPE IF EXISTS comm_state;
CREATE TYPE comm_state AS ENUM ('current', 'not_in_use');
CREATE TABLE ftp_comm (id serial PRIMARY KEY,
probe_cmd varchar(64),
username varchar(32),
passwd varchar(32),
ip_address varchar(32),
port int,
comm_enabled boolean,
state comm_state,
last_modified timestamp);
INSERT INTO ftp_comm (probe_cmd,
     username, passwd,
     ip_address, port, comm_enabled,
     state, last_modified)
     VALUES 
     ('/upmu_data/',
     'user', 'pass',
     'upmuIP', 21,
     true, 'current', NOW());
