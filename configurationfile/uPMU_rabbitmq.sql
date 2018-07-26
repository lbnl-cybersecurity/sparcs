\connect upmugateway
SET default_tablespace=upmu_datastore;
DROP TABLE IF EXISTS rabbitmq;
CREATE TABLE rabbitmq (id serial PRIMARY KEY,
routekey varchar(32),
exchange varchar(32),
vhost varchar(32),
db_user varchar(32),
passwd varchar(32),
archiving_enabled boolean,
ip_address varchar(32),
port int,
location varchar(32),
last_modified timestamp);
INSERT INTO rabbitmq (
     routekey, exchange, vhost,
     db_user, passwd,
     archiving_enabled,
     ip_address, port, location,
     last_modified)
     VALUES 
     ('power.upmu_1.modbus.psl.', 'ha-metric', '',
     'user', 'pass',
     true, 'yourserver.com', 5672, 'nameofupmu',
     NOW());
