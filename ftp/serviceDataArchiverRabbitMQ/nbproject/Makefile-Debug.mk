#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Environment
MKDIR=mkdir
CP=cp
GREP=grep
NM=nm
CCADMIN=CCadmin
RANLIB=ranlib
CC=gcc
CCC=g++
CXX=g++
FC=gfortran
AS=as

# Macros
CND_PLATFORM=GNU-Linux
CND_DLIB_EXT=so
CND_CONF=Debug
CND_DISTDIR=dist
CND_BUILDDIR=build

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/_ext/498f5db2/uPMUgmtime.o \
	${OBJECTDIR}/_ext/498f5db2/upmuStructKeyValueListProtobuf.pb.o \
	${OBJECTDIR}/serviceDataArchiverRabbitMQ.o \
	${OBJECTDIR}/serviceSpecificStats.o \
	${OBJECTDIR}/upmuDataProtobufRabbitMQ.pb.o


# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=-std=c++11
CXXFLAGS=-std=c++11

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=-L../../../distributions/datastax-cpp-driver-5d694f4/build -lprotobuf -lcassandra -lrabbitmq -lSimpleAmqpClient

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libserviceDataArchiverRabbitMQ.${CND_DLIB_EXT}

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libserviceDataArchiverRabbitMQ.${CND_DLIB_EXT}: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${LINK.cc} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libserviceDataArchiverRabbitMQ.${CND_DLIB_EXT} ${OBJECTFILES} ${LDLIBSOPTIONS} -shared -fPIC

${OBJECTDIR}/_ext/498f5db2/uPMUgmtime.o: ../uPMUgateway/uPMUgmtime.cpp
	${MKDIR} -p ${OBJECTDIR}/_ext/498f5db2
	${RM} "$@.d"
	$(COMPILE.cc) -g -I../uPMUgateway -I/home/distributions/cassandra/datastax-cpp-driver/include -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/498f5db2/uPMUgmtime.o ../uPMUgateway/uPMUgmtime.cpp

${OBJECTDIR}/_ext/498f5db2/upmuStructKeyValueListProtobuf.pb.o: ../uPMUgateway/upmuStructKeyValueListProtobuf.pb.cc
	${MKDIR} -p ${OBJECTDIR}/_ext/498f5db2
	${RM} "$@.d"
	$(COMPILE.cc) -g -I../uPMUgateway -I/home/distributions/cassandra/datastax-cpp-driver/include -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/498f5db2/upmuStructKeyValueListProtobuf.pb.o ../uPMUgateway/upmuStructKeyValueListProtobuf.pb.cc

${OBJECTDIR}/serviceDataArchiverRabbitMQ.o: serviceDataArchiverRabbitMQ.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -I../uPMUgateway -I/home/distributions/cassandra/datastax-cpp-driver/include -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/serviceDataArchiverRabbitMQ.o serviceDataArchiverRabbitMQ.cpp

${OBJECTDIR}/serviceSpecificStats.o: serviceSpecificStats.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -I../uPMUgateway -I/home/distributions/cassandra/datastax-cpp-driver/include -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/serviceSpecificStats.o serviceSpecificStats.cpp

${OBJECTDIR}/upmuDataProtobufRabbitMQ.pb.o: upmuDataProtobufRabbitMQ.pb.cc
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -I../uPMUgateway -I/home/distributions/cassandra/datastax-cpp-driver/include -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/upmuDataProtobufRabbitMQ.pb.o upmuDataProtobufRabbitMQ.pb.cc

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
