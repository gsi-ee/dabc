
##
#
# Project level Makefile
#
##


include $(XDAQ_ROOT)/config/mfAutoconf.rules

Project=dabc

include $(XDAQ_ROOT)/config/mfDefs.$(XDAQ_OS)



# these are the base packages:
PackagesBase=\
	nodecontrol



# default:
Packages= $(PackagesBase)


# packages with examples:
#PackagesExamples=\
#    examples/modulesbnet


ifeq ($(Set), base)
Packages= $(PackagesBase)
endif



#ifeq ($(Set), examples)
#Packages=$(PackagesExamples)
#endif

ifeq ($(Set), all)
Packages= $(PackagesBase)
endif



#include $(XDAQ_ROOT)/config/Makefile.rules
include $(DABCSYS)/controls/xdaq/config/Makefile.rules