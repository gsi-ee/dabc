## normally should be like this for every module, but can be specific



KERNELMODULESDIR = /lib/modules/$(shell uname -r)/build
ifneq ($(realpath $(KERNELMODULESDIR)),)
#USEABB    = true
endif


ifdef USEABB 

ABB_INCLUDES :=

ifdef DABCMAINMAKE
ABBDIR = plugins/abb
else
ABBDIR = .
ABB_INCLUDES += $(ABBDIR)
endif

#this is MPRACE components location
ifndef MPRACE_DIR
MPRACE_DIR      = $(ABBDIR)/linuxdrivers/mprace
endif

MPRACE_LIBDIR   = $(MPRACE_DIR)/lib
MPRACE_INCLUDES = $(MPRACE_DIR)/include 
MPRACE_DEFINITIONS = 
MPRACE_LIBS     = -lmprace 
MPRACE_LIBNAME  = $(LIB_PREFIX)mprace.$(DllSuf)


#this is Mannheim pciDriver components location
ifndef PCIDRIVER_DIR
PCIDRIVER_DIR      = $(ABBDIR)/linuxdrivers/pciDriver2
endif

PCIDRIVER_LIBDIR   = $(PCIDRIVER_DIR)/lib
PCIDRIVER_INCLUDES = $(PCIDRIVER_DIR)/include 
PCIDRIVER_DEFINITIONS = 
PCIDRIVER_LIBS     = -lpcidriver 
PCIDRIVER_LIBNAME  = $(LIB_PREFIX)pcidriver.$(DllSuf)

include $(DABCSYS)/config/Makefile.config


ABBDIRPCI       = $(ABBDIR)/pci
ABBDIRABB       = $(ABBDIR)/abb
ABBDIRS         = $(ABBDIR)/src
ABBTESTDIR      = $(ABBDIR)/test

DABCABB_LIBNAME  = $(LIB_PREFIX)DabcAbb
DABCABB_LIB      = $(TGTDLLPATH)/$(DABCABB_LIBNAME).$(DllSuf)

## must be similar for every module

ABB_DEFINITIONS = $(MPRACE_DEFINITIONS) $(PCIDRIVER_DEFINITIONS)
ABB_INCLUDES    += $(MPRACE_INCLUDES) $(PCIDRIVER_INCLUDES)

ABB_H           = $(wildcard $(ABBDIRPCI)/*.$(HedSuf)) $(wildcard $(ABBDIRABB)/*.$(HedSuf))
ABB_S           = $(wildcard $(ABBDIRS)/*.$(SrcSuf))
ABB_O           = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(ABB_S))
ABB_D           = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(ABB_S))

# used in the main Makefile

ALLHDRS         += $(patsubst $(ABBDIR)/%.h, $(DABCINCPATH)/%.h, $(ABB_H))
ALLDEPENDENC    += $(ABB_D) 


libs:: pcidriver mprace $(DABCABB_LIB)

clean:: mprace-clean pcidriver-clean

pcidriver:
	@cd $(PCIDRIVER_DIR); $(MAKE) all 
ifdef DABCMAINMAKE
	@cp -vf $(PCIDRIVER_LIBDIR)/*.so $(TGTDLLPATH)
endif

pcidriver-clean:
	@cd $(PCIDRIVER_DIR); $(MAKE) clean


mprace:
	@cd $(MPRACE_DIR); $(MAKE) all; 
ifdef DABCMAINMAKE
	@cp -vf $(MPRACE_LIBDIR)/*.so $(TGTDLLPATH)
endif

mprace-clean:
	@cd $(MPRACE_DIR); $(MAKE) clean; 

##### local rules #####

$(DABCINCPATH)/%.h: $(ABBDIR)/%.h
	@echo "Header: $@"
	@cp -f $< $@

$(DABCABB_LIB):   $(ABB_O)
	@$(MakeLib) $(DABCABB_LIBNAME) "$(ABB_O)" $(TGTDLLPATH) "-L$(MPRACE_LIBDIR) $(MPRACE_LIBS) -L$(PCIDRIVER_LIBDIR) $(PCIDRIVER_LIBS)"

$(ABB_O) $(ABB_D): INCLUDES += $(ABB_INCLUDES) 
$(ABB_O) $(ABB_D): DEFINITIONS += $(ABB_DEFINITIONS)

include $(DABCSYS)/config/Makefile.rules

endif
