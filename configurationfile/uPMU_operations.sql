\connect upmugateway
SET default_tablespace=upmu_datastore;
DROP TABLE IF EXISTS operations;
CREATE TABLE operations (id serial PRIMARY KEY,
state varchar(32),
device varchar(32),
session_id int,
component varchar(32),
key varchar(64),
value_type varchar(32),
value_string varchar(64),
value_float float,
value_int int,
last_modified timestamp);
