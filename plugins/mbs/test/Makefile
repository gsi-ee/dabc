include $(DABCSYS)/config/Makefile.config

TEST1_S        = $(_topdir_)mbstest.$(SrcSuf)
TEST1_O        = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(TEST1_S))
TEST1_D        = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(TEST1_S))
TEST1_EXE      = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%$(ExeSuf), $(TEST1_S))

TESTLIB_NAME   = $(LIB_PREFIX)MbsTest1
TESTLIB_LIB    = $(TGTDLLPATH)/$(TESTLIB_NAME).$(DllSuf)

TESTLIB_S      = $(_topdir_)mbsservtest.$(SrcSuf)
TESTLIB_O      = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(TESTLIB_S))
TESTLIB_D      = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(TESTLIB_S))

ALLDEPENDENC  += $(TEST1_D)

libs:: $(TESTLIB_LIB)

exes::  $(TEST1_EXE)

$(TEST1_EXE):  $(TEST1_O)
	$(LD) $(LDFLAGS) $(TEST1_O) $(LIBS_CORESET) -lDabcMbs $(OutPutOpt) $(TEST1_EXE)
ifneq ($(wildcard ~/run/mbs),)
	cp -f $(TEST1_EXE) $(DABCDLLPATH)/*.$(DllSuf) ~/run/mbs
endif
	@echo "$@  done"

$(TESTLIB_LIB):   $(TESTLIB_O)
	@$(MakeLib) $(TESTLIB_NAME) "$(TESTLIB_O)" $(TGTDLLPATH)


include $(DABCSYS)/config/Makefile.rules

