\connect upmugateway
SET default_tablespace=upmu_datastore;
DROP TABLE IF EXISTS service;
DROP TYPE IF EXISTS exec_mode;
CREATE TYPE exec_mode AS ENUM ('debug', 'production');
CREATE TABLE service (id serial PRIMARY KEY,
service_name varchar(64),
service_implementation varchar(64),
service_exec_mode exec_mode,
service_lib_path varchar(128),
service_debug_lib_path varchar(128),
service_accepts_data boolean,
service_mask smallint,
service_active boolean,
service_queue_depth smallint,
last_modified timestamp);
INSERT INTO service (service_name,
     service_implementation, service_exec_mode,
     service_lib_path, service_debug_lib_path,
     service_accepts_data, service_mask,
     service_active, service_queue_depth,
     last_modified) VALUES
     ('serviceDataDiscovery', 'libserviceDataDiscovery.so', 
	  'debug',
	  '/home/mcp/NetBeansProject_uPMU/serviceDataDiscovery/dist/Debug/GNU-Linux',
	  '../serviceDataDiscovery/dist/Debug/GNU-Linux',
	  false, 0, true, 16, NOW());
INSERT INTO service (service_name,
     service_implementation, service_exec_mode,
     service_lib_path, service_debug_lib_path,
     service_accepts_data, service_mask,
     service_active, service_queue_depth,
     last_modified) VALUES
     ('serviceDataQA', 'libserviceDataQA.so', 
	  'debug',
          '/home/mcp/NetBeansProject_uPMU/serviceDataQA/dist/Debug/GNU-Linux',
          '../serviceDataQA/dist/Debug/GNU-Linux',
          true, 0, false, 16, NOW());
INSERT INTO service (service_name,
     service_implementation, service_exec_mode,
     service_lib_path, service_debug_lib_path,
     service_accepts_data, service_mask,
     service_active, service_queue_depth,
     last_modified) VALUES
     ('serviceDataMonitor', 'libserviceDataMonitor.so',
	  'debug', 
          '/home/mcp/NetBeansProject_uPMU/serviceDataMonitor/dist/Debug/GNU-Linux',
          '../serviceDataMonitor/dist/Debug/GNU-Linux',
          true, 0, false, 16, NOW());
INSERT INTO service (service_name,
     service_implementation, service_exec_mode,
     service_lib_path, service_debug_lib_path,
     service_accepts_data, service_mask,
     service_active, service_queue_depth,
     last_modified) VALUES
     ('serviceDataArchiver', 'libserviceDataArchiver.so',
	  'debug', 
          '/home/mcp/NetBeansProject_uPMU/serviceDataArchiver/dist/Debug/GNU-Linux', 
          '../serviceDataArchiver/dist/Debug/GNU-Linux', 
          true, 0, true, 16, NOW());
INSERT INTO service (service_name,
     service_implementation, service_exec_mode,
     service_lib_path, service_debug_lib_path,
     service_accepts_data, service_mask,
     service_active, service_queue_depth,
     last_modified) VALUES
     ('serviceDataArchiverRabbitMQ', 'libserviceDataArchiverRabbitMQ.so',
          'debug',
          '/home/mcp/NetBeansProject_uPMU/serviceDataArchiverRabbitMQ/dist/Debug/GNU-Linux',
          '../serviceDataArchiverRabbitMQ/dist/Debug/GNU-Linux',
          true, 0, true, 16, NOW());
