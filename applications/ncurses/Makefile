ifndef noncurses
ifneq ($(wildcard /usr/inlcude/ncurses.h),)
ifneq ($(wildcard /usr/include/menu.h),)


include $(DABCSYS)/config/Makefile.config

ifdef DABCMAINMAKE
NCURSESDIR = applications/ncurses/
DABC_SHELLEXE     = $(DABCBINPATH)/dabc_shell
else
NCURSESDIR = 
DABC_SHELLEXE     = dabc_shell
endif

DABC_SHELLEXES    = $(NCURSESDIR)dabc_shell.$(SrcSuf)
DABC_SHELLEXEO    = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(DABC_SHELLEXES))
DABC_SHELLEXED    = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(DABC_SHELLEXES))


ALLDEPENDENC += $(DABC_SHELLEXED)

exes::  $(DABC_SHELLEXE)

clean::
	rm -f $(DABC_SHELLEXE)

$(DABC_SHELLEXE) : $(DABC_SHELLEXEO) $(DABCBASE_LIB) 
	$(LD) -Wl,--no-as-needed -O $(DABC_SHELLEXEO) $(LIBS_CORESET) -lpthread -ldl -lrt -lmenu -lncurses -o $(DABC_SHELLEXE)


include $(DABCSYS)/config/Makefile.rules

endif
endif
endif
