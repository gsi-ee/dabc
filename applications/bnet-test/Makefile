# this is generic makefile for DABC application

include $(DABCSYS)/config/Makefile.config

# enable this define to use active buffer board
# DEFINITIONS += __USE_ABB__

ifdef DABCMAINMAKE
BNETTESTDIR = applications/bnet-test/
else
BNETTESTDIR = 
INCLUDES += .
endif

BNETBINPATH          = $(DABCSYS)/plugins/bnet

BNETTEST_LIBNAME     = $(LIB_PREFIX)BnetTest
BNETTEST_LIB         = $(TGTDLLPATH)/$(BNETTEST_LIBNAME).$(DllSuf)

BNETTEST_S           = $(wildcard $(BNETTESTDIR)*.$(SrcSuf))
BNETTEST_O           = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(BNETTEST_S))
BNETTEST_D           = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(BNETTEST_S))

ALLDEPENDENC        += $(BNETTEST_D)


libs::  $(BNETTEST_LIB)

$(BNETTEST_LIB):  $(BNETTEST_O)
	@$(MakeLib) $(BNETTEST_LIBNAME) "$(BNETTEST_O)" $(TGTDLLPATH)
ifneq ($(wildcard ~/run/bnet),)
	cp -f $(BNETTEST_LIB) ~/run/bnet
	cp -f $(DABCDLLPATH)/*.$(DllSuf) ~/run/bnet 
endif
	@echo "$@  done"

include $(DABCSYS)/config/Makefile.rules
