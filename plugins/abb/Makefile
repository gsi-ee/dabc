## normally should be like this for every module, but can be specific

ifdef DABCMAINMAKE
ABBDIR = plugins/abb
else
ABBDIR = .
INCLUDES += $(ABBDIR)
endif

#this is MPRACE components location
ifndef MPRACE_DIR
MPRACE_DIR      = $(ABBDIR)/linuxdrivers/mprace
endif

MPRACE_LIBDIR   = $(MPRACE_DIR)/lib
MPRACE_INCLUDES = $(MPRACE_DIR)/include 
MPRACE_DEFINITIONS = 
MPRACE_LIBS     = -lmprace 

#this is Mannheim pciDriver components location
ifndef PCIDRIVER_DIR
PCIDRIVER_DIR      = $(ABBDIR)/linuxdrivers/pciDriver2
endif

PCIDRIVER_LIBDIR   = $(PCIDRIVER_DIR)/lib
PCIDRIVER_INCLUDES = $(PCIDRIVER_DIR)/include 
PCIDRIVER_DEFINITIONS = 
PCIDRIVER_LIBS     = -lpcidriver 

ifneq ($(wildcard $(MPRACE_LIBDIR)/libmprace.so),)
ifneq ($(wildcard $(PCIDRIVER_LIBDIR)/libpcidriver.so),)
USEMPRACE   = true
endif
endif

ifdef USEMPRACE

include $(DABCSYS)/config/Makefile.config


ABBDIRPCI       = $(ABBDIR)/pci
ABBDIRABB       = $(ABBDIR)/abb
ABBDIRS         = $(ABBDIR)/src
ABBTESTDIR      = $(ABBDIR)/test

DABCABB_LIBNAME  = $(LIB_PREFIX)DabcAbb
DABCABB_LIB      = $(TGTDLLPATH)/$(DABCABB_LIBNAME).$(DllSuf)

## must be similar for every module

ABB_DEFINITIONS = $(MPRACE_DEFINITIONS) $(PCIDRIVER_DEFINITIONS)
ABB_INCLUDES    = $(MPRACE_INCLUDES) $(PCIDRIVER_INCLUDES)

ABB_H           = $(wildcard $(ABBDIRPCI)/*.$(HedSuf)) $(wildcard $(ABBDIRABB)/*.$(HedSuf))
ABB_S           = $(wildcard $(ABBDIRS)/*.$(SrcSuf))
ABB_O           = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(ABB_S))
ABB_D           = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(ABB_S))

# used in the main Makefile

ALLHDRS         += $(patsubst $(ABBDIR)/%.h, $(DABCINCPATH)/%.h, $(ABB_H))
ALLDEPENDENC    += $(ABB_D) 
CREATE_DIRS     += $(TGTDLLPATH)
LIBS_PLUGINS    += $(DABCABB_LIB)

libs:: $(DABCABB_LIB)

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
