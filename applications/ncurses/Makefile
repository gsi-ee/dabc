ifndef noncurses
ifneq ($(wildcard /usr/include/ncurses/ncurses.h),)
ifneq ($(wildcard /usr/include/ncurses/menu.h),)


include $(DABCSYS)/config/Makefile.config

ifdef DABCMAINMAKE
NCURSESDIR        = applications/ncurses/
DABC_SHELLEXE     = $(DABCBINPATH)/dabc_shell
else
NCURSESDIR        =
DABC_SHELLEXE     = dabc_shell
endif

DABC_SHELLEXES    = $(NCURSESDIR)dabc_shell.$(SrcSuf)
DABC_SHELLEXEO    = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(DABC_SHELLEXES))
DABC_SHELLEXED    = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(DABC_SHELLEXES))


ALLDEPENDENC += $(DABC_SHELLEXED)

exes::  $(DABC_SHELLEXE)

clean::
	$(RM) $(DABC_SHELLEXE)

$(DABC_SHELLEXE) : $(DABC_SHELLEXEO) $(DABCBASE_LIB)
	$(LD) $(LDFLAGSPRE) -O $(DABC_SHELLEXEO) $(LIBS_CORESET) -lmenu -lncurses -o $(DABC_SHELLEXE)

$(DABC_SHELLEXEO): INCLUDES += /usr/include/ncurses

include $(DABCSYS)/config/Makefile.rules

endif
endif
endif
