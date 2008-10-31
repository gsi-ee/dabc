# in the beginning initialise all OFED variables and check if one has OFED installed
# to change default configuration, it is enough to change OFED_DIR and OFED_LIBDIR
# the rest can remain as is
# If required, support of VERBS MCAST can be enabled  

ifndef OFED_DIR
OFED_DIR        = /usr/local/ofed
endif

ifndef OFED_LIBDIR
OFED_LIBDIR     = $(OFED_DIR)/lib64
endif

ifndef OFED_MCAST
OFED_MCAST      = true
endif
 
ifndef OFED_LIBS
OFED_LIBS       = -libverbs
endif

ifeq ($(OFED_MCAST), true)
OFED_INCLUDES += /usr/include/infiniband
OFED_LIBS     += -lopensm -losmcomp -losmvendor -lrt
endif

# switch to enable OFED library support 
ifndef USEOFEDVERBS
ifneq ($(wildcard $(OFED_LIBDIR))$(wildcard $(OFED_DIR)),)
USEOFEDVERBS    = true
endif
endif


ifdef USEOFEDVERBS

include $(DABCSYS)/config/Makefile.config

ifdef DABCMAINMAKE
VERBSDIR = plugins/verbs
else
VERBSDIR = .
INCLUDES += $(VERBSDIR)
endif


VERBSDIRI         = $(VERBSDIR)/verbs
VERBSDIRS         = $(VERBSDIR)/src

VERBS_NOTLIBF = 

DABCVERBS_LIBNAME  = $(LIB_PREFIX)DabcVerbs
DABCVERBS_LIB      = $(TGTDLLPATH)/$(DABCVERBS_LIBNAME).$(DllSuf)

## must be similar for every module

VERBS_DEFINITIONS = 
ifneq ($(OFED_MCAST), true)
VERBS_DEFINITIONS += __NO_MULTICAST__
VERBS_NOTLIBF += $(VERBSDIRI)/OpenSM.h $(VERBSDIRS)/OpenSM.cxx
endif

VERBS_H           = $(filter-out $(VERBS_NOTLIBF), $(wildcard $(VERBSDIRI)/*.$(HedSuf)))
VERBS_S           = $(filter-out $(VERBS_NOTLIBF), $(wildcard $(VERBSDIRS)/*.$(SrcSuf)))
VERBS_O           = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(VERBS_S))
VERBS_D           = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(VERBS_S))

ALLHDRS          +=  $(patsubst $(VERBSDIR)/%.h, $(DABCINCPATH)/%.h, $(VERBS_H))
ALLDEPENDENC     += $(VERBS_D) 
CREATE_DIRS      += $(TGTDLLPATH)
LIBS_PLUGINS     += $(DABCVERBS_LIB)
LIBS_EXTRA       += -L$(OFED_LIBDIR) $(OFED_LIBS)  

libs:: $(DABCVERBS_LIB)

##### local rules #####

$(DABCINCPATH)/%.h: $(VERBSDIR)/%.h
	@echo "Header: $@" 
	@cp -f $< $@

$(DABCVERBS_LIB):   $(VERBS_O)
	@$(MakeLib) $(DABCVERBS_LIBNAME) "$(VERBS_O)" $(TGTDLLPATH) "-L$(OFED_LIBDIR) $(OFED_LIBS)"

$(VERBS_O) $(VERBS_D): INCLUDES += $(OFED_INCLUDES)
$(VERBS_O) $(VERBS_D): DEFINITIONS += $(VERBS_DEFINITIONS)

include $(DABCSYS)/config/Makefile.rules

endif
