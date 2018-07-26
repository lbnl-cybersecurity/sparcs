\connect upmugateway
SET default_tablespace=upmu_datastore;
DROP TABLE IF EXISTS upmu_data_file;
DROP TYPE IF EXISTS file_state;
CREATE TYPE file_state AS ENUM ('abandoned', 'processing', 'released');
CREATE TABLE upmu_data_file (id serial PRIMARY KEY,
file_name varchar(32) NOT NULL,
directory serial references upmu_data_directory(id) NOT NULL,
state file_state NOT NULL,
access_start_time timestamp NOT NULL,
release_time timestamp);
