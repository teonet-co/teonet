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
CND_PLATFORM=GNU-Linux-x86
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
	${OBJECTDIR}/src/config/conf.o \
	${OBJECTDIR}/src/config/opt.o \
	${OBJECTDIR}/src/ev_mgr.o \
	${OBJECTDIR}/src/hotkeys.o \
	${OBJECTDIR}/src/modules/vpn.o \
	${OBJECTDIR}/src/net_arp.o \
	${OBJECTDIR}/src/net_com.o \
	${OBJECTDIR}/src/net_core.o \
	${OBJECTDIR}/src/teonet.o \
	${OBJECTDIR}/src/utils/string_arr.o \
	${OBJECTDIR}/src/utils/utils.o

# Test Directory
TESTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}/tests

# Test Files
TESTFILES= \
	${TESTDIR}/TestFiles/f1

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
LDLIBSOPTIONS=-Lembedded/libpbl/src -lev -lpbl -lconfuse -luuid -ltuntap

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libteonet.${CND_DLIB_EXT}

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libteonet.${CND_DLIB_EXT}: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${LINK.c} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libteonet.${CND_DLIB_EXT} ${OBJECTFILES} ${LDLIBSOPTIONS} -shared -fPIC

${OBJECTDIR}/src/config/conf.o: src/config/conf.c 
	${MKDIR} -p ${OBJECTDIR}/src/config
	${RM} "$@.d"
	$(COMPILE.c) -g -Isrc -Iembedded/libpbl/src -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/config/conf.o src/config/conf.c

${OBJECTDIR}/src/config/opt.o: src/config/opt.c 
	${MKDIR} -p ${OBJECTDIR}/src/config
	${RM} "$@.d"
	$(COMPILE.c) -g -Isrc -Iembedded/libpbl/src -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/config/opt.o src/config/opt.c

${OBJECTDIR}/src/ev_mgr.o: src/ev_mgr.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.c) -g -Isrc -Iembedded/libpbl/src -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/ev_mgr.o src/ev_mgr.c

${OBJECTDIR}/src/hotkeys.o: src/hotkeys.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.c) -g -Isrc -Iembedded/libpbl/src -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/hotkeys.o src/hotkeys.c

${OBJECTDIR}/src/modules/vpn.o: src/modules/vpn.c 
	${MKDIR} -p ${OBJECTDIR}/src/modules
	${RM} "$@.d"
	$(COMPILE.c) -g -Isrc -Iembedded/libpbl/src -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/modules/vpn.o src/modules/vpn.c

${OBJECTDIR}/src/net_arp.o: src/net_arp.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.c) -g -Isrc -Iembedded/libpbl/src -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/net_arp.o src/net_arp.c

${OBJECTDIR}/src/net_com.o: src/net_com.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.c) -g -Isrc -Iembedded/libpbl/src -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/net_com.o src/net_com.c

${OBJECTDIR}/src/net_core.o: src/net_core.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.c) -g -Isrc -Iembedded/libpbl/src -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/net_core.o src/net_core.c

${OBJECTDIR}/src/teonet.o: src/teonet.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.c) -g -Isrc -Iembedded/libpbl/src -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/teonet.o src/teonet.c

${OBJECTDIR}/src/utils/string_arr.o: src/utils/string_arr.c 
	${MKDIR} -p ${OBJECTDIR}/src/utils
	${RM} "$@.d"
	$(COMPILE.c) -g -Isrc -Iembedded/libpbl/src -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/utils/string_arr.o src/utils/string_arr.c

${OBJECTDIR}/src/utils/utils.o: src/utils/utils.c 
	${MKDIR} -p ${OBJECTDIR}/src/utils
	${RM} "$@.d"
	$(COMPILE.c) -g -Isrc -Iembedded/libpbl/src -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/utils/utils.o src/utils/utils.c

# Subprojects
.build-subprojects:

# Build Test Targets
.build-tests-conf: .build-conf ${TESTFILES}
${TESTDIR}/TestFiles/f1: ${TESTDIR}/tests/teonet_tst.o ${OBJECTFILES:%.o=%_nomain.o}
	${MKDIR} -p ${TESTDIR}/TestFiles
	${LINK.c}   -o ${TESTDIR}/TestFiles/f1 $^ ${LDLIBSOPTIONS} 


${TESTDIR}/tests/teonet_tst.o: tests/teonet_tst.c 
	${MKDIR} -p ${TESTDIR}/tests
	${RM} "$@.d"
	$(COMPILE.c) -g -Isrc -Iembedded/libpbl/src -I. -MMD -MP -MF "$@.d" -o ${TESTDIR}/tests/teonet_tst.o tests/teonet_tst.c


${OBJECTDIR}/src/config/conf_nomain.o: ${OBJECTDIR}/src/config/conf.o src/config/conf.c 
	${MKDIR} -p ${OBJECTDIR}/src/config
	@NMOUTPUT=`${NM} ${OBJECTDIR}/src/config/conf.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.c) -g -Isrc -Iembedded/libpbl/src -fPIC  -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/config/conf_nomain.o src/config/conf.c;\
	else  \
	    ${CP} ${OBJECTDIR}/src/config/conf.o ${OBJECTDIR}/src/config/conf_nomain.o;\
	fi

${OBJECTDIR}/src/config/opt_nomain.o: ${OBJECTDIR}/src/config/opt.o src/config/opt.c 
	${MKDIR} -p ${OBJECTDIR}/src/config
	@NMOUTPUT=`${NM} ${OBJECTDIR}/src/config/opt.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.c) -g -Isrc -Iembedded/libpbl/src -fPIC  -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/config/opt_nomain.o src/config/opt.c;\
	else  \
	    ${CP} ${OBJECTDIR}/src/config/opt.o ${OBJECTDIR}/src/config/opt_nomain.o;\
	fi

${OBJECTDIR}/src/ev_mgr_nomain.o: ${OBJECTDIR}/src/ev_mgr.o src/ev_mgr.c 
	${MKDIR} -p ${OBJECTDIR}/src
	@NMOUTPUT=`${NM} ${OBJECTDIR}/src/ev_mgr.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.c) -g -Isrc -Iembedded/libpbl/src -fPIC  -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/ev_mgr_nomain.o src/ev_mgr.c;\
	else  \
	    ${CP} ${OBJECTDIR}/src/ev_mgr.o ${OBJECTDIR}/src/ev_mgr_nomain.o;\
	fi

${OBJECTDIR}/src/hotkeys_nomain.o: ${OBJECTDIR}/src/hotkeys.o src/hotkeys.c 
	${MKDIR} -p ${OBJECTDIR}/src
	@NMOUTPUT=`${NM} ${OBJECTDIR}/src/hotkeys.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.c) -g -Isrc -Iembedded/libpbl/src -fPIC  -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/hotkeys_nomain.o src/hotkeys.c;\
	else  \
	    ${CP} ${OBJECTDIR}/src/hotkeys.o ${OBJECTDIR}/src/hotkeys_nomain.o;\
	fi

${OBJECTDIR}/src/modules/vpn_nomain.o: ${OBJECTDIR}/src/modules/vpn.o src/modules/vpn.c 
	${MKDIR} -p ${OBJECTDIR}/src/modules
	@NMOUTPUT=`${NM} ${OBJECTDIR}/src/modules/vpn.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.c) -g -Isrc -Iembedded/libpbl/src -fPIC  -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/modules/vpn_nomain.o src/modules/vpn.c;\
	else  \
	    ${CP} ${OBJECTDIR}/src/modules/vpn.o ${OBJECTDIR}/src/modules/vpn_nomain.o;\
	fi

${OBJECTDIR}/src/net_arp_nomain.o: ${OBJECTDIR}/src/net_arp.o src/net_arp.c 
	${MKDIR} -p ${OBJECTDIR}/src
	@NMOUTPUT=`${NM} ${OBJECTDIR}/src/net_arp.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.c) -g -Isrc -Iembedded/libpbl/src -fPIC  -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/net_arp_nomain.o src/net_arp.c;\
	else  \
	    ${CP} ${OBJECTDIR}/src/net_arp.o ${OBJECTDIR}/src/net_arp_nomain.o;\
	fi

${OBJECTDIR}/src/net_com_nomain.o: ${OBJECTDIR}/src/net_com.o src/net_com.c 
	${MKDIR} -p ${OBJECTDIR}/src
	@NMOUTPUT=`${NM} ${OBJECTDIR}/src/net_com.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.c) -g -Isrc -Iembedded/libpbl/src -fPIC  -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/net_com_nomain.o src/net_com.c;\
	else  \
	    ${CP} ${OBJECTDIR}/src/net_com.o ${OBJECTDIR}/src/net_com_nomain.o;\
	fi

${OBJECTDIR}/src/net_core_nomain.o: ${OBJECTDIR}/src/net_core.o src/net_core.c 
	${MKDIR} -p ${OBJECTDIR}/src
	@NMOUTPUT=`${NM} ${OBJECTDIR}/src/net_core.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.c) -g -Isrc -Iembedded/libpbl/src -fPIC  -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/net_core_nomain.o src/net_core.c;\
	else  \
	    ${CP} ${OBJECTDIR}/src/net_core.o ${OBJECTDIR}/src/net_core_nomain.o;\
	fi

${OBJECTDIR}/src/teonet_nomain.o: ${OBJECTDIR}/src/teonet.o src/teonet.c 
	${MKDIR} -p ${OBJECTDIR}/src
	@NMOUTPUT=`${NM} ${OBJECTDIR}/src/teonet.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.c) -g -Isrc -Iembedded/libpbl/src -fPIC  -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/teonet_nomain.o src/teonet.c;\
	else  \
	    ${CP} ${OBJECTDIR}/src/teonet.o ${OBJECTDIR}/src/teonet_nomain.o;\
	fi

${OBJECTDIR}/src/utils/string_arr_nomain.o: ${OBJECTDIR}/src/utils/string_arr.o src/utils/string_arr.c 
	${MKDIR} -p ${OBJECTDIR}/src/utils
	@NMOUTPUT=`${NM} ${OBJECTDIR}/src/utils/string_arr.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.c) -g -Isrc -Iembedded/libpbl/src -fPIC  -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/utils/string_arr_nomain.o src/utils/string_arr.c;\
	else  \
	    ${CP} ${OBJECTDIR}/src/utils/string_arr.o ${OBJECTDIR}/src/utils/string_arr_nomain.o;\
	fi

${OBJECTDIR}/src/utils/utils_nomain.o: ${OBJECTDIR}/src/utils/utils.o src/utils/utils.c 
	${MKDIR} -p ${OBJECTDIR}/src/utils
	@NMOUTPUT=`${NM} ${OBJECTDIR}/src/utils/utils.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.c) -g -Isrc -Iembedded/libpbl/src -fPIC  -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/utils/utils_nomain.o src/utils/utils.c;\
	else  \
	    ${CP} ${OBJECTDIR}/src/utils/utils.o ${OBJECTDIR}/src/utils/utils_nomain.o;\
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
	${RM} ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libteonet.${CND_DLIB_EXT}

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
