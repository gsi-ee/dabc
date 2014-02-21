## normally should be like this for every plugin
## one can extract complete plugin subdirectory and compile it independent from rest

include $(DABCSYS)/config/Makefile.config


ifdef DABCMAINMAKE
MBSDIR = plugins/mbs
MBSPRINT_EXE      = $(DABCBINPATH)/mbsprint
else
MBSDIR = .
INCLUDES += $(MBSDIR)
MBSPRINT_EXE      = mbsprint
endif

MBSDIRI         = $(MBSDIR)/mbs
MBSDIRS         = $(MBSDIR)/src

DABCMBS_LIBNAME  = $(LIB_PREFIX)DabcMbs
DABCMBS_LIB      = $(TGTDLLPATH)/$(DABCMBS_LIBNAME).$(DllSuf)

DABCMBS_H        = $(wildcard $(MBSDIRI)/*.$(HedSuf))
DABCMBS_S        = $(wildcard $(MBSDIRS)/*.$(SrcSuf))
DABCMBS_O        = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(DABCMBS_S))
DABCMBS_D        = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(DABCMBS_S))

MBSPRINT_EXES    = $(MBSDIR)/mbsprint.$(SrcSuf)
MBSPRINT_EXEO    = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(MBSPRINT_EXES))
MBSPRINT_EXED    = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(MBSPRINT_EXES))


# used in the main Makefile

ALLHDRS           += $(patsubst $(MBSDIR)/%.h, $(DABCINCPATH)/%.h, $(DABCMBS_H))
ALLDEPENDENC      += $(DABCMBS_D) $(MBSPRINT_EXED)

libs:: $(DABCMBS_LIB)

exes::  $(MBSPRINT_EXE)

clean::
	rm -f $(MBSPRINT_EXE)

##### local rules #####

$(DABCINCPATH)/%.h: $(MBSDIR)/%.h
	@echo "Header: $@" 
	@cp -f $< $@


$(DABCMBS_LIB): LDFLAGS += -lrt

$(DABCMBS_LIB):   $(DABCMBS_O)
	@$(MakeLib) $(DABCMBS_LIBNAME) "$(DABCMBS_O)" $(TGTDLLPATH) 

$(MBSPRINT_EXE) : $(MBSPRINT_EXEO) $(DABCBASE_LIB) $(DABCMBS_LIB)
	$(LD) -O $(MBSPRINT_EXEO) -Wl,--no-as-needed $(LIBS_CORESET) -lDabcMbs -o $(MBSPRINT_EXE)

include $(DABCSYS)/config/Makefile.rules
