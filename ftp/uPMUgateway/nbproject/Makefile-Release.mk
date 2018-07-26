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
CND_CONF=Release
CND_DISTDIR=dist
CND_BUILDDIR=build

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/dbConnection.o \
	${OBJECTDIR}/main.o \
	${OBJECTDIR}/serviceCommonStats.o \
	${OBJECTDIR}/serviceInstance.o \
	${OBJECTDIR}/serviceInstanceList.o \
	${OBJECTDIR}/serviceOperationsLoggerCassandra.o \
	${OBJECTDIR}/serviceOperationsLoggerPostgres.o \
	${OBJECTDIR}/uPMUgmtime.o \
	${OBJECTDIR}/upmuStructKeyValueListProtobuf.pb.o


# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=
CXXFLAGS=

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/upmugateway

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/upmugateway: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${LINK.cc} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/upmugateway ${OBJECTFILES} ${LDLIBSOPTIONS}

${OBJECTDIR}/dbConnection.o: dbConnection.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/dbConnection.o dbConnection.cpp

${OBJECTDIR}/main.o: main.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/main.o main.cpp

${OBJECTDIR}/serviceCommonStats.o: serviceCommonStats.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/serviceCommonStats.o serviceCommonStats.cpp

${OBJECTDIR}/serviceInstance.o: serviceInstance.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/serviceInstance.o serviceInstance.cpp

${OBJECTDIR}/serviceInstanceList.o: serviceInstanceList.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/serviceInstanceList.o serviceInstanceList.cpp

${OBJECTDIR}/serviceOperationsLoggerCassandra.o: serviceOperationsLoggerCassandra.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/serviceOperationsLoggerCassandra.o serviceOperationsLoggerCassandra.cpp

${OBJECTDIR}/serviceOperationsLoggerPostgres.o: serviceOperationsLoggerPostgres.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/serviceOperationsLoggerPostgres.o serviceOperationsLoggerPostgres.cpp

${OBJECTDIR}/uPMUgmtime.o: uPMUgmtime.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/uPMUgmtime.o uPMUgmtime.cpp

${OBJECTDIR}/upmuStructKeyValueListProtobuf.pb.o: upmuStructKeyValueListProtobuf.pb.cc 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/upmuStructKeyValueListProtobuf.pb.o upmuStructKeyValueListProtobuf.pb.cc

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}
	${RM} ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/upmugateway

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
