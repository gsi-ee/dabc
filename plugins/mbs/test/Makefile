include $(DABCSYS)/config/Makefile.config

TEST1_S        = $(_topdir_)mbstest.$(SrcSuf)
TEST1_O        = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(TEST1_S))
TEST1_D        = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(TEST1_S))
TEST1_EXE      = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%$(ExeSuf), $(TEST1_S))

TEST2_S        = $(_topdir_)mbsservtest.$(SrcSuf)
TEST2_O        = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(TEST2_S))
TEST2_D        = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(TEST2_S))
TEST2_EXE      = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%$(ExeSuf), $(TEST2_S))

ALLDEPENDENC  += $(TEST1_D) $(TEST2_D)

exes::  $(TEST1_EXE) $(TEST2_EXE)

$(TEST1_EXE):  $(TEST1_O)
	$(LD) $(LDFLAGS) $(TEST1_O) $(LIBS_CORESET) -lDabcMbs $(OutPutOpt) $(TEST1_EXE)
ifneq ($(wildcard ~/run/mbs),)
	cp -f $(TEST1_EXE) $(DABCDLLPATH)/*.$(DllSuf) ~/run/mbs
endif
	@echo "$@  done"

$(TEST2_EXE):  $(TEST2_O)
	$(LD) $(LDFLAGS) $(TEST2_O) $(LIBS_CORESET) -lDabcMbs $(OutPutOpt) $(TEST2_EXE)
ifneq ($(wildcard ~/run/mbs),)
	cp -f $(TEST2_EXE) $(DABCDLLPATH)/*.$(DllSuf) ~/run/mbs
endif
	@echo "$@  done"

include $(DABCSYS)/config/Makefile.rules

