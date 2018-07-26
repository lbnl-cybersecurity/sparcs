\connect upmugateway
SET default_tablespace=upmu_datastore;
DROP TABLE IF EXISTS upmu_data_directory CASCADE;
DROP TYPE IF EXISTS directory_state;
CREATE TYPE directory_state AS ENUM ('current', 'abandoned', 'processed', 'unknown');
CREATE TABLE upmu_data_directory (id serial PRIMARY KEY,
year smallint NOT NULL,
month smallint NOT NULL,
day smallint NOT NULL,
hour smallint NOT NULL,
dir_name varchar NOT NULL,
state directory_state NOT NULL,
discovery_time timestamp NOT NULL,
release_time timestamp);
