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
	${OBJECTDIR}/_ext/498f5db2/dbConnection.o \
	${OBJECTDIR}/_ext/498f5db2/serviceOperationsLoggerCassandra.o \
	${OBJECTDIR}/_ext/498f5db2/uPMUgmtime.o \
	${OBJECTDIR}/dataFileInstance.o \
	${OBJECTDIR}/serviceDataDiscovery.o \
	${OBJECTDIR}/serviceSpecificStats.o \
	${OBJECTDIR}/upmuFtpAccess.o

# Test Directory
TESTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}/tests

# Test Files
TESTFILES= \
	${TESTDIR}/TestFiles/f1

# Test Object Files
TESTOBJECTFILES= \
	${TESTDIR}/tests/newsimpletest.o

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
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libserviceDataDiscovery.${CND_DLIB_EXT}

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libserviceDataDiscovery.${CND_DLIB_EXT}: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${LINK.cc} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libserviceDataDiscovery.${CND_DLIB_EXT} ${OBJECTFILES} ${LDLIBSOPTIONS} -shared -fPIC

${OBJECTDIR}/_ext/498f5db2/dbConnection.o: ../uPMUgateway/dbConnection.cpp 
	${MKDIR} -p ${OBJECTDIR}/_ext/498f5db2
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/498f5db2/dbConnection.o ../uPMUgateway/dbConnection.cpp

${OBJECTDIR}/_ext/498f5db2/serviceOperationsLoggerCassandra.o: ../uPMUgateway/serviceOperationsLoggerCassandra.cpp 
	${MKDIR} -p ${OBJECTDIR}/_ext/498f5db2
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/498f5db2/serviceOperationsLoggerCassandra.o ../uPMUgateway/serviceOperationsLoggerCassandra.cpp

${OBJECTDIR}/_ext/498f5db2/uPMUgmtime.o: ../uPMUgateway/uPMUgmtime.cpp 
	${MKDIR} -p ${OBJECTDIR}/_ext/498f5db2
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/498f5db2/uPMUgmtime.o ../uPMUgateway/uPMUgmtime.cpp

${OBJECTDIR}/dataFileInstance.o: dataFileInstance.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/dataFileInstance.o dataFileInstance.cpp

${OBJECTDIR}/serviceDataDiscovery.o: serviceDataDiscovery.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/serviceDataDiscovery.o serviceDataDiscovery.cpp

${OBJECTDIR}/serviceSpecificStats.o: serviceSpecificStats.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/serviceSpecificStats.o serviceSpecificStats.cpp

${OBJECTDIR}/upmuFtpAccess.o: upmuFtpAccess.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/upmuFtpAccess.o upmuFtpAccess.cpp

# Subprojects
.build-subprojects:

# Build Test Targets
.build-tests-conf: .build-tests-subprojects .build-conf ${TESTFILES}
.build-tests-subprojects:

${TESTDIR}/TestFiles/f1: ${TESTDIR}/tests/newsimpletest.o ${OBJECTFILES:%.o=%_nomain.o}
	${MKDIR} -p ${TESTDIR}/TestFiles
	${LINK.cc}   -o ${TESTDIR}/TestFiles/f1 $^ ${LDLIBSOPTIONS} 


${TESTDIR}/tests/newsimpletest.o: tests/newsimpletest.cpp 
	${MKDIR} -p ${TESTDIR}/tests
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -I. -MMD -MP -MF "$@.d" -o ${TESTDIR}/tests/newsimpletest.o tests/newsimpletest.cpp


${OBJECTDIR}/_ext/498f5db2/dbConnection_nomain.o: ${OBJECTDIR}/_ext/498f5db2/dbConnection.o ../uPMUgateway/dbConnection.cpp 
	${MKDIR} -p ${OBJECTDIR}/_ext/498f5db2
	@NMOUTPUT=`${NM} ${OBJECTDIR}/_ext/498f5db2/dbConnection.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.cc) -O2 -fPIC  -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/498f5db2/dbConnection_nomain.o ../uPMUgateway/dbConnection.cpp;\
	else  \
	    ${CP} ${OBJECTDIR}/_ext/498f5db2/dbConnection.o ${OBJECTDIR}/_ext/498f5db2/dbConnection_nomain.o;\
	fi

${OBJECTDIR}/_ext/498f5db2/serviceOperationsLoggerCassandra_nomain.o: ${OBJECTDIR}/_ext/498f5db2/serviceOperationsLoggerCassandra.o ../uPMUgateway/serviceOperationsLoggerCassandra.cpp 
	${MKDIR} -p ${OBJECTDIR}/_ext/498f5db2
	@NMOUTPUT=`${NM} ${OBJECTDIR}/_ext/498f5db2/serviceOperationsLoggerCassandra.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.cc) -O2 -fPIC  -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/498f5db2/serviceOperationsLoggerCassandra_nomain.o ../uPMUgateway/serviceOperationsLoggerCassandra.cpp;\
	else  \
	    ${CP} ${OBJECTDIR}/_ext/498f5db2/serviceOperationsLoggerCassandra.o ${OBJECTDIR}/_ext/498f5db2/serviceOperationsLoggerCassandra_nomain.o;\
	fi

${OBJECTDIR}/_ext/498f5db2/uPMUgmtime_nomain.o: ${OBJECTDIR}/_ext/498f5db2/uPMUgmtime.o ../uPMUgateway/uPMUgmtime.cpp 
	${MKDIR} -p ${OBJECTDIR}/_ext/498f5db2
	@NMOUTPUT=`${NM} ${OBJECTDIR}/_ext/498f5db2/uPMUgmtime.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.cc) -O2 -fPIC  -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/498f5db2/uPMUgmtime_nomain.o ../uPMUgateway/uPMUgmtime.cpp;\
	else  \
	    ${CP} ${OBJECTDIR}/_ext/498f5db2/uPMUgmtime.o ${OBJECTDIR}/_ext/498f5db2/uPMUgmtime_nomain.o;\
	fi

${OBJECTDIR}/dataFileInstance_nomain.o: ${OBJECTDIR}/dataFileInstance.o dataFileInstance.cpp 
	${MKDIR} -p ${OBJECTDIR}
	@NMOUTPUT=`${NM} ${OBJECTDIR}/dataFileInstance.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.cc) -O2 -fPIC  -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/dataFileInstance_nomain.o dataFileInstance.cpp;\
	else  \
	    ${CP} ${OBJECTDIR}/dataFileInstance.o ${OBJECTDIR}/dataFileInstance_nomain.o;\
	fi

${OBJECTDIR}/serviceDataDiscovery_nomain.o: ${OBJECTDIR}/serviceDataDiscovery.o serviceDataDiscovery.cpp 
	${MKDIR} -p ${OBJECTDIR}
	@NMOUTPUT=`${NM} ${OBJECTDIR}/serviceDataDiscovery.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.cc) -O2 -fPIC  -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/serviceDataDiscovery_nomain.o serviceDataDiscovery.cpp;\
	else  \
	    ${CP} ${OBJECTDIR}/serviceDataDiscovery.o ${OBJECTDIR}/serviceDataDiscovery_nomain.o;\
	fi

${OBJECTDIR}/serviceSpecificStats_nomain.o: ${OBJECTDIR}/serviceSpecificStats.o serviceSpecificStats.cpp 
	${MKDIR} -p ${OBJECTDIR}
	@NMOUTPUT=`${NM} ${OBJECTDIR}/serviceSpecificStats.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.cc) -O2 -fPIC  -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/serviceSpecificStats_nomain.o serviceSpecificStats.cpp;\
	else  \
	    ${CP} ${OBJECTDIR}/serviceSpecificStats.o ${OBJECTDIR}/serviceSpecificStats_nomain.o;\
	fi

${OBJECTDIR}/upmuFtpAccess_nomain.o: ${OBJECTDIR}/upmuFtpAccess.o upmuFtpAccess.cpp 
	${MKDIR} -p ${OBJECTDIR}
	@NMOUTPUT=`${NM} ${OBJECTDIR}/upmuFtpAccess.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.cc) -O2 -fPIC  -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/upmuFtpAccess_nomain.o upmuFtpAccess.cpp;\
	else  \
	    ${CP} ${OBJECTDIR}/upmuFtpAccess.o ${OBJECTDIR}/upmuFtpAccess_nomain.o;\
	fi

# Run Test Targets
.test-conf:
	@if [ "${TEST}" = "" ]; \
	then  \
	    ${TESTDIR}/TestFiles/f1 || true; \
	else  \
	    ./${TEST} || true; \
	fi

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}
	${RM} ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libserviceDataDiscovery.${CND_DLIB_EXT}

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
