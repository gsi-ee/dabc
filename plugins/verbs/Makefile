## normally should be like this for every module, but can be specific

VERBSDIR          = verbs
VERBSDIRI         = $(VERBSDIR)/inc
VERBSDIRS         = $(VERBSDIR)/src

VERBS_NOTLIBF     =

DABCVERBS_LIBNAME  = $(LIB_PREFIX)DabcVerbs
DABCVERBS_LIB      = $(DABCDLLPATH)/$(DABCVERBS_LIBNAME).$(DllSuf)

## must be similar for every module

VERBS_DEFINITIONS = 
ifneq ($(OFED_MCAST), true)
VERBS_DEFINITIONS += __NO_MULTICAST__
VERBS_NOTLIBF += $(VERBSDIRI)/VerbsOsm.h $(VERBSDIRS)/VerbsOsm.cxx
endif

VERBS_H           = $(filter-out $(VERBS_NOTLIBF), $(wildcard $(VERBSDIRI)/*.$(HedSuf)))
VERBS_S           = $(filter-out $(VERBS_NOTLIBF), $(wildcard $(VERBSDIRS)/*.$(SrcSuf)))
VERBS_O           = $(VERBS_S:.$(SrcSuf)=.$(ObjSuf))
VERBS_DEP         = $(VERBS_O:.$(ObjSuf)=.$(DepSuf))

# used in the main Makefile

DISTRFILES         += $(VERBS_S) $(VERBS_H) $(VERBS_NOTLIBF)

ifdef USEOFEDVERBS

ALLHDRS +=  $(patsubst $(VERBSDIRI)/%.h, $(DABCINCPATH)/%.h, $(VERBS_H))

BUILDDABCLIBS      += $(DABCVERBS_LIB)

ALLDEPENDENC       += $(VERBS_DEP) 

all-verbs:  $(DABCVERBS_LIB)

else 

all-verbs: 
	@echo "Verbs not build"

endif

##### local rules #####

$(DABCINCPATH)/%.h: $(VERBSDIRI)/%.h
	@cp -f $< $@

$(DABCVERBS_LIB):   $(VERBS_O)
	@$(MakeLib) $(DABCVERBS_LIBNAME) "$(VERBS_O)" $(DABCDLLPATH) "-L$(OFED_LIBDIR) $(OFED_LIBS)"

clean-verbs:
	@rm -f $(VERBS_O) $(VERBS_DEP)
	@$(CleanLib) $(DABCVERBS_LIBNAME) $(DABCDLLPATH)

clean:: clean-verbs

########### extra roles #############
$(VERBS_O) $(VERBS_DEP): INCLUDES += $(OFED_INCLUDES:%=-I%)
$(VERBS_O) $(VERBS_DEP): DEFINITIONS += $(VERBS_DEFINITIONS:%=-D%)
