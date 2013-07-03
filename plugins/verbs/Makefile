ifndef noverbs

# switch to enable OFED library support 
ifndef USEOFEDVERBS
ifneq ($(wildcard /usr/lib64/libibverbs.*)$(wildcard /usr/lib/libibverbs.*),)
ifneq ($(wildcard /usr/include/infiniband/*.h),)
USEOFEDVERBS    = true
endif
endif
endif

ifeq ($(USEOFEDVERBS),true)

include $(DABCSYS)/config/Makefile.config

OFED_INCLUDES   =
OFED_LIBS       = -libverbs

ifdef DABCMAINMAKE
VERBSDIR = plugins/verbs
else
VERBSDIR = .
OFED_INCLUDES += .
endif

ifeq ($(OFED_MCAST), true)
OFED_INCLUDES += /usr/include/infiniband
OFED_LIBS     += -lopensm -losmcomp -losmvendor -lrt
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

libs:: $(DABCVERBS_LIB)

##### local rules #####

$(DABCINCPATH)/%.h: $(VERBSDIR)/%.h
	@echo "Header: $@" 
	@cp -f $< $@

$(DABCVERBS_LIB): LDFLAGS += $(OFED_LIBS)

$(DABCVERBS_LIB):   $(VERBS_O)
	@$(MakeLib) $(DABCVERBS_LIBNAME) "$(VERBS_O)" $(TGTDLLPATH)

$(VERBS_O) $(VERBS_D): INCLUDES += $(OFED_INCLUDES)
$(VERBS_O) $(VERBS_D): DEFINITIONS += $(VERBS_DEFINITIONS)

include $(DABCSYS)/config/Makefile.rules

endif

endif