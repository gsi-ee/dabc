
#ifndef EPICS_BASE
#EPICS_BASE=/u/epics/EPICS-3.14.9/base-3.14.9
#endif
#
#
#ifndef EPICS_EXTENSIONS
#EPICS_EXTENSIONS=/misc/adamczew/epics/myextensions 
#endif



ifndef USEEPICS

ifdef EPICS_BASE
ifdef EPICS_EXTENSIONS
ifneq ($(wildcard $(EPICS_EXTENSIONS)/lib/linux-x86/libezca.*)$(wildcard $(EPICS_EXTENSIONS)/lib/linux-x86_64/libezca.*),)
USEEPICS    = true
endif
endif
endif


endif

ifeq ($(USEEPICS),true)




include $(DABCSYS)/config/Makefile.config

## normally should be like this for every module, but can be specific



#DOOPTIMIZATION = false





# libezca.so

ifdef DABCMAINMAKE
EZCADIR = plugins/ezca
INCLUDES += plugins/ezca
else
EZCADIR = .
INCLUDES += .
endif


EZCA_DIRI         = $(EZCADIR)/ezca
EZCA_DIRS         = $(EZCADIR)/src

EZCA_LIBNAME     = $(LIB_PREFIX)DabcEzca
EZCA_LIB         = $(TGTDLLPATH)/$(EZCA_LIBNAME).$(DllSuf)

## must be similar for every module

EZCA_H           = $(wildcard $(EZCA_DIRI)/*.$(HedSuf))
EZCA_S           = $(wildcard $(EZCA_DIRS)/*.$(SrcSuf))
EZCA_O           = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(EZCA_S))
EZCA_D           = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(EZCA_S))

# used in the main Makefile

ALLDEPENDENC    += $(EZCA_D)

libs:: $(EZCA_LIB)

clean::
	@rm -f $(EZCA_LIB)


##### local rules #####

$(EZCA_O) $(EZCA_D) : INCLUDES += $(EPICS_BASE)/include $(EPICS_BASE)/include/os/Linux $(EPICS_EXTENSIONS)/include 

# TODO: link to the EZCA library
#$(EZCA_LIB) : LDFLAGS += -L... -lezca

$(EZCA_LIB):  $(EZCA_O)
	@$(MakeLib) $(EZCA_LIBNAME) "$(EZCA_O)" $(TGTDLLPATH)

########### extra rules #############

include $(DABCSYS)/config/Makefile.rules

endif
