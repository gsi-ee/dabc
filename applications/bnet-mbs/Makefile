include $(DABCSYS)/config/Makefile.config

ifdef DABCMAINMAKE
BNETMBS_DIR = applications/bnet-mbs/
else
BNETMBS_DIR = 
INCLUDES += .
endif

BNETBINPATH         = $(DABCSYS)/plugins/bnet/scripts

BNETMBS_LIBNAME     = $(LIB_PREFIX)BnetMbs
BNETMBS_LIB         = $(TGTDLLPATH)/$(BNETMBS_LIBNAME).$(DllSuf)

BNETMBS_EXES        = $(BNETMBS_DIR)genmbsfiles.$(SrcSuf)
BNETMBS_EXEO        = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(BNETMBS_EXES))
BNETMBS_EXED        = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(BNETMBS_EXES))
BNETMBS_EXE         = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%$(ExeSuf), $(BNETMBS_EXES))

BNETMBS_S           = $(filter-out $(BNETMBS_EXES), $(wildcard $(BNETMBS_DIR)*.$(SrcSuf)))
BNETMBS_O           = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(BNETMBS_S))
BNETMBS_D           = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(BNETMBS_S))

ALLDEPENDENC        += $(BNETMBS_D) $(BNETMBS_EXED)

CREATE_DIRS         += $(TGTDLLPATH)

libs:: $(BNETMBS_LIB) 

exes:: $(BNETMBS_EXE) 

$(BNETMBS_LIB):   $(BNETMBS_O)
	@$(MakeLib) $(BNETMBS_LIBNAME) "$(BNETMBS_O)" $(TGTDLLPATH)

$(BNETMBS_EXE):  $(BNETMBS_EXEO) $(BNETMBS_LIB) $(BNETMBS_DIR)Makefile $(BNETMBS_DIR)genmbsfiles.sh
	$(LD) $(LDFLAGS) $(BNETMBS_EXEO) $(LIBS_CORESET) -lDabcMbs $(OutPutOpt) $(BNETMBS_EXE)
ifneq ($(wildcard ~/run/bnet/mbs),)
	cp -f $(BNETMBS_EXE) $(BNETMBS_LIB) $(DABCDLLPATH)/*.so $(BNETBINPATH)/* $(BNETMBS_DIR)*.sh ~/run/bnet/mbs
endif

include $(DABCSYS)/config/Makefile.rules
