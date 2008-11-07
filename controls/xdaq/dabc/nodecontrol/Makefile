# $Id: Makefile,v 1.4 2008/06/13 08:13:15 adamczew Exp adamczew $


include $(XDAQ_ROOT)/config/mfAutoconf.rules
include $(XDAQ_ROOT)/config/mfDefs.$(XDAQ_OS)

#CCDefines += -D__NODAPL__
# use this to disable link against udapl lib (for other nodes)

CCDefines += -D__XDAQVERSION__=$(XDAQ_VERSION)
# need to check xdaq version because application group changes 37
# some includes are changed from 311 on

#this is OFED components location
OFED_DIR        = /usr/local/ofed
OFED_LIBDIR     = $(OFED_DIR)/lib64
OFED_MCAST      = false
OFED_INCLUDES   = $(OFED_DIR)/include 
OFED_LIBS       = -libverbs

ifeq ($(OFED_MCAST), true)
OFED_INCLUDES += $(OFED_DIR)/include/infiniband
OFED_LIBS     += -lopensm -losmcomp -losmvendor -lrt
endif

# switch to enable OFED library support 
ifneq ($(wildcard $(OFED_LIBDIR))$(wildcard $(OFED_DIR)),)
CCDefines += -D__OFEDVERBS__
endif




XDAQ_ROOT_DEV=$(DABCSYS)/controls/xdaq
Project=dabc
Package=nodecontrol


Sources= DABCRegistry.cc DABCDimServer.cc  DABCApplication.cc DABCParameter.cc DABCManager.cc  DABCNode.cc DABCWorker.cc DABCController.cc nameParser.cc DABC_controlV.cc 


IncludeDirs = \
	$(DIMDIR)/dim \
	$(DABCSYS)/include/ 

LibraryDirs = \
	$(DIMDIR)/linux \
	$(DABCSYS)/lib 
UserCFlags =
UserCCFlags = 
UserDynamicLinkFlags =
UserStaticLinkFlags =
UserExecutableLinkFlags =

# These libraries can be platform specific and
# potentially need conditional processing
#

Libraries = 

#
# Compile the source files and create a shared library
#
DynamicLibrary= DabcXDAQControl

StaticLibrary=

ifdef Executable
Libraries=toolbox xdata xcept xoap xerces-c log4cplus mimetic asyncresolv
Executables= $(Executable).cc
endif

include $(DABCSYS)/controls/xdaq/config/Makefile.rules
