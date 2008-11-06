# this is generic makefile for DABC application

include $(DABCSYS)/config/Makefile.config

# enable this define to use active buffer board
# DEFINITIONS += __USE_ABB__


BNETBINPATH           = $(DABCSYS)/plugins/bnet/scripts

BNETTEST_LIBNAME     = $(LIB_PREFIX)BnetTest
BNETTEST_LIB         = $(TGTDLLPATH)/$(BNETTEST_LIBNAME).$(DllSuf)

BNETTEST_S           = $(wildcard $(_topdir_)*.$(SrcSuf))
BNETTEST_O           = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(BNETTEST_S))
BNETTEST_D           = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(BNETTEST_S))

ALLDEPENDENC        += $(BNETTEST_D)
CREATE_DIRS         += $(TGTDLLPATH)

libs::  $(BNETTEST_LIB)

$(BNETTEST_LIB):  $(BNETTEST_O) $(DABCDLLPATH)/*.$(DllSuf)
	@$(MakeLib) $(BNETTEST_LIBNAME) "$(BNETTEST_O)" $(TGTDLLPATH)
ifneq ($(wildcard ~/run/bnet),)
	cp -f $(BNETTEST_LIB) ~/run/bnet
	cp -f $(DABCDLLPATH)/*.$(DllSuf) $(DABCBINPATH)/bnet_exe ~/run/bnet
	cp -f $(BNETBINPATH)/* ~/run/bnet
endif
	@echo "$@  done"


include $(DABCSYS)/config/Makefile.rules
