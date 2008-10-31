include $(DABCSYS)/config/Makefile.config

TEST1_S        = $(_topdir_)test1.$(SrcSuf)
TEST1_O        = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(TEST1_S))
TEST1_D        = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(TEST1_S))
TEST1_EXE      = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%$(ExeSuf), $(TEST1_S))

ALLDEPENDENC   += $(TEST1_D)

exes::  $(TEST1_EXE)


$(TEST1_EXE):  $(TEST1_O)
	$(LD) $(LDFLAGS) $(TEST1_O) $(LIBS_CORESET) -lDabcSctrl $(OutPutOpt) $(TEST1_EXE)
ifneq ($(wildcard ~/run/test1),)
	cp -f $(TEST1_EXE) *.sh *.txt $(DABCDLLPATH)/*.$(DllSuf) ~/run/test1
endif
	@echo "$@  done"

include $(DABCSYS)/config/Makefile.rules
