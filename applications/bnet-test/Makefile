# this is generic makefile for DABC application

include $(DABCSYS)/config/Makefile.config

ifdef DABCMAINMAKE
BNETTESTDIR = applications/bnet-test/
else
BNETTESTDIR = 
INCLUDES += .
endif

BNETTEST_LIBNAME     = $(LIB_PREFIX)BnetTest
BNETTEST_LIB         = $(BNETTESTDIR)$(BNETTEST_LIBNAME).$(DllSuf)

BNETTEST_S           = $(wildcard $(BNETTESTDIR)*.$(SrcSuf))
BNETTEST_O           = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(BNETTEST_S))
BNETTEST_D           = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(BNETTEST_S))

ALLDEPENDENC        += $(BNETTEST_D)

libs::  $(BNETTEST_LIB)

clean::
	rm -f $(BNETTEST_LIB)
	echo Clean $(BNETTEST_LIB)

$(BNETTEST_LIB):  $(BNETTEST_O)
	@$(MakeLib) $(BNETTEST_LIBNAME) "$(BNETTEST_O)" $(BNETTESTDIR)
	@echo "$@  done"

include $(DABCSYS)/config/Makefile.rules
