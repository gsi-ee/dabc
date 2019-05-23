## normally should be like this for every plugin
## one can extract complete plugin subdirectory and compile it independent from rest

include $(DABCSYS)/config/Makefile.config


ifdef DABCMAINMAKE
MBSDIR = plugins/mbs
MBSPRINT_EXE      = $(DABCBINPATH)/mbsprint
MBSCMD_EXE        = $(DABCBINPATH)/mbscmd
else
MBSDIR = .
INCLUDES += $(MBSDIR)
MBSPRINT_EXE      = mbsprint
MBSCMD_EXE        = mbscmd
endif

MBSDIRI         = $(MBSDIR)/mbs
MBSDIRS         = $(MBSDIR)/src

# move it into Makefile.config, while mbs plugin used in many other places 
#DABCMBS_LIBNAME  = $(LIB_PREFIX)DabcMbs
#DABCMBS_LIB      = $(TGTDLLPATH)/$(DABCMBS_LIBNAME).$(DllSuf)

DABCMBS_H        = $(wildcard $(MBSDIRI)/*.$(HedSuf))
DABCMBS_S        = $(wildcard $(MBSDIRS)/*.$(SrcSuf))
DABCMBS_O        = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(DABCMBS_S))
DABCMBS_D        = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(DABCMBS_S))

MBSPRINT_EXES    = $(MBSDIR)/utils/mbsprint.$(SrcSuf)
MBSPRINT_EXEO    = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(MBSPRINT_EXES))
MBSPRINT_EXED    = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(MBSPRINT_EXES))

MBSCMD_EXES      = $(MBSDIR)/utils/mbscmd.$(SrcSuf)
MBSCMD_EXEO      = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(MBSCMD_EXES))
MBSCMD_EXED      = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(MBSCMD_EXES))

# used in the main Makefile

ALLHDRS           += $(patsubst $(MBSDIR)/%.h, $(DABCINCPATH)/%.h, $(DABCMBS_H))
ALLDEPENDENC      += $(DABCMBS_D) $(MBSPRINT_EXED) $(MBSCMD_EXED)

libs:: $(DABCMBS_LIB)

exes::  $(MBSPRINT_EXE) $(MBSCMD_EXE)

clean::
	rm -f $(MBSPRINT_EXE) $(MBSCMD_EXE)

##### local rules #####

$(DABCINCPATH)/%.h: $(MBSDIR)/%.h
	@echo "Header: $@" 
	@cp -f $< $@


$(DABCMBS_LIB):   $(DABCMBS_O)
	@$(MakeLib) $(DABCMBS_LIBNAME) "$(DABCMBS_O)" $(TGTDLLPATH) "-lDabcCore"

$(MBSPRINT_EXE) : $(MBSPRINT_EXEO) $(DABCBASE_LIB) $(DABCMBS_LIB)
	$(LD) -O $(MBSPRINT_EXEO) -Wl,--no-as-needed $(LIBS_CORESET) -lDabcMbs -o $(MBSPRINT_EXE)

$(MBSCMD_EXE) : $(MBSCMD_EXEO) $(DABCBASE_LIB) $(DABCMBS_LIB)
	$(LD) -O $(MBSCMD_EXEO) -Wl,--no-as-needed $(LIBS_CORESET) -lDabcMbs -o $(MBSCMD_EXE)

include $(DABCSYS)/config/Makefile.rules
