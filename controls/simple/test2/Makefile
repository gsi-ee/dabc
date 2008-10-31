include $(DABCSYS)/config/Makefile.config

## must be similar for every module

TEST2_S        = $(_topdir_)test2.$(SrcSuf)
TEST2_O        = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(TEST2_S))
TEST2_D        = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(TEST2_S))
TEST2_EXE      = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%$(ExeSuf), $(TEST2_S))

ALLDEPENDENC   += $(TEST2_D)

exes::  $(TEST2_EXE)

$(TEST2_EXE):  $(TEST2_O)
	$(LD) $(LDFLAGS) $(TEST2_O) $(LIBS_CORESET) -lDabcSctrl $(OutPutOpt) $(TEST2_EXE)
ifneq ($(wildcard ~/run/test2),)
	cp -f $(TEST2_EXE) *.sh *.txt $(DABCDLLPATH)/*.$(DllSuf) ~/run/test2
endif
	@echo "$@  done"

include $(DABCSYS)/config/Makefile.rules
