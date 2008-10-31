include $(DABCSYS)/config/Makefile.config

BNETBINPATH           = $(DABCSYS)/plugins/bnet/scripts

BNETTEST_LIBNAME     = $(LIB_PREFIX)BnetWorker
BNETTEST_LIB         = $(TGTDLLPATH)/$(BNETTEST_LIBNAME).$(DllSuf)

BNETTEST_EXES        = genmbsfiles.$(SrcSuf)
BNETTEST_EXEO        = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(BNETTEST_EXES))
BNETTEST_EXED        = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(BNETTEST_EXES))
BNETTEST_EXE         = $(patsubst %.$(SrcSuf), $(TGTBINPATH)/%$(ExeSuf), $(BNETTEST_EXES))

BNETTEST_S           = $(filter-out $(BNETTEST_EXES), $(wildcard *.$(SrcSuf)))
BNETTEST_O           = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(BNETTEST_S))
BNETTEST_D           = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(BNETTEST_S))

ALLDEPENDENC        += $(BNETTEST_D) $(BNETTEST_EXED)

CREATE_DIRS         += $(TGTDLLPATH) $(TGTBINPATH)

libs:: $(BNETTEST_LIB) 

exes:: $(BNETTEST_EXE) 

$(BNETTEST_LIB):   $(BNETTEST_O)
	@$(MakeLib) $(BNETTEST_LIBNAME) "$(BNETTEST_O)" $(TGTDLLPATH)

$(BNETTEST_EXE):  $(BNETTEST_EXEO) $(BNETTEST_LIB) Makefile genmbsfiles.sh
	$(LD) $(LDFLAGS) $(BNETTEST_EXEO) $(LIBS_FULLSET) -L$(TGTDLLPATH) -lBnetWorker $(OutPutOpt) $(BNETTEST_EXE)
ifneq ($(wildcard ~/run/bnet/mbs),)
	cp -f $(BNETTEST_EXE) $(BNETTEST_LIB) $(DABCDLLPATH)/*.so $(BNETBINPATH)/* *.sh ~/run/bnet/mbs
endif

include $(DABCSYS)/config/Makefile.rules
