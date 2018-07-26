/*
 */

/* 
 * File:   domainAndMessageTypes.h
 * Author: mcp
 *
 * Created on May 5, 2016, 4:56 AM
 */

#ifndef DOMAINANDMESSAGETYPES_H
#define DOMAINANDMESSAGETYPES_H

/* These constants are used to define domain and message types associated
 with protocol buffers that are logged to the cassandra database.  Taken
 together they will indicate what tyoe of protocol buffer message is being
 logged.  Note that domain refers to the functional entity that is attempting
 to log data and message refers to the specific form and content of the message
 itself.  So, it is likely that several tuples that share the same message
 type will, in fact, have the same protocol buffer format.*/

#define DOMAIN_CONFIGURATION 0
#define DOMAIN_OPERATIONS 1
#define DOMAIN_DATA 2
#define DOMAIN_METADATA 3
#define DOMAIN_ANNOTATIONS 4

#define MSG_UPMU_DATA 0
#define MSG_KEY_VALUE_LIST 1



#endif /* DOMAINANDMESSAGETYPES_H */

