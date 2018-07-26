/*
 */

/* 
 * File:   serviceCommands.h
 * Author: mcp
 *
 * Created on January 25, 2016, 4:01 PM
 */

#ifndef SERVICECOMMAND_H
#define SERVICECOMMAND_H

#define ADMIN_DOMAIN 0
#define SERVICE_SPARE_0 0
#define SERVICE_STOP 1

#define SERVICE_INSTANCE_TEMPLATE_DOMAIN 1

#define SERVICE_DATA_DISCOVERY_DOMAIN 2

#define COMMAND_BODY_BYTE_LEN 128

typedef struct serviceCommand {
    unsigned char serviceDomain;
    unsigned char serviceSpecific;
    unsigned int byteLen;
    unsigned char * cmdBuffer;
} SERVICE_COMMAND;

#endif /* SERVICECOMMAND_H */

