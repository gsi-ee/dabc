## normally should be like this for every plugin
## one can extract complete plugin subdirectory and compile it independent from rest

include $(DABCSYS)/config/Makefile.config

ifdef DABCMAINMAKE
MBSDIR = plugins/mbs
else
MBSDIR = .
INCLUDES += $(MBSDIR)
endif

MBSDIRI         = $(MBSDIR)/mbs
MBSDIRS         = $(MBSDIR)/src

DABCMBS_LIBNAME  = $(LIB_PREFIX)DabcMbs
DABCMBS_LIB      = $(TGTDLLPATH)/$(DABCMBS_LIBNAME).$(DllSuf)

DABCMBS_H           = $(wildcard $(MBSDIRI)/*.$(HedSuf))
DABCMBS_S           = $(wildcard $(MBSDIRS)/*.$(SrcSuf))
DABCMBS_O           = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(DABCMBS_S))
DABCMBS_D           = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(DABCMBS_S))


# used in the main Makefile

ALLHDRS           += $(patsubst $(MBSDIR)/%.h, $(DABCINCPATH)/%.h, $(DABCMBS_H))
ALLDEPENDENC      += $(DABCMBS_D)

libs:: $(DABCMBS_LIB)

##### local rules #####

$(DABCINCPATH)/%.h: $(MBSDIR)/%.h
	@echo "Header: $@" 
	@cp -f $< $@


$(DABCMBS_LIB): LDFLAGS += -lrt

$(DABCMBS_LIB):   $(DABCMBS_O)
	@$(MakeLib) $(DABCMBS_LIBNAME) "$(DABCMBS_O)" $(TGTDLLPATH) 

########### extra rules #############
$(MBS_O) $(MBS_D) : DEFINITIONS += $(MBSAPI_DEFINITIONS) 

include $(DABCSYS)/config/Makefile.rules
