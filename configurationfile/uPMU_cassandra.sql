\connect upmugateway
SET default_tablespace=upmu_datastore;
DROP TABLE IF EXISTS cassandra;
CREATE TABLE cassandra (id serial PRIMARY KEY,
keyspace varchar(32),
data_table varchar(32),
metadata_table varchar(32),
configuration_table varchar(32),
annotations_table varchar(32),
operations_table varchar(32),
archiving_enabled boolean,
ip_address varchar(32),
port int,
last_modified timestamp);
INSERT INTO cassandra (keyspace,
     data_table, metadata_table,
     configuration_table, annotations_table,
     operations_table,
     ip_address, port, archiving_enabled,
     last_modified)
     VALUES 
     ('upmu_devel', 'upmu_data',
     'upmu_metadata', 'upmu_configuration',
     'upmu_annotations', 'upmu_operations',
     'cassandraip',
     9042, true, NOW());
