include $(DABCSYSCORE)/config/Makefile.config

BNETBINPATH           = $(DABCSYSCORE)/bnet/bin

ifdef USEMPRACE
#INCLUDES += $(ABB_INCLUDES:%=-I%) 
DEFINITIONS += -D__USE_ABB__
endif

BNETTEST_LIBNAME     = $(LIB_PREFIX)BnetWorker
BNETTEST_LIB         = $(BNETTEST_LIBNAME).$(DllSuf)

BNETTEST_S           = $(wildcard *.$(SrcSuf))
BNETTEST_O           = $(BNETTEST_S:.$(SrcSuf)=.$(ObjSuf))

BNETTEST_DEP         =  $(BNETTEST_O:.$(ObjSuf)=.$(DepSuf))
BNETTEST_EDEP        =  $(BNETTEST_EXEO:.$(ObjSuf)=.$(DepSuf))

ALLDEPENDENC        += $(BNETTEST_DEP) $(BNETTEST_EDEP)

all:  $(BNETTEST_LIB)

$(BNETTEST_LIB):  $(BNETTEST_O) $(wildcard $(BNETBINPATH)/*) $(wildcard $(DABCDLLPATH)/*.so) Makefile
	@$(MakeLib) $(BNETTEST_LIBNAME) "$(BNETTEST_O)" $(CURDIR) "-L$(BNETBINPATH) -lDabcBnet"
ifneq ($(wildcard ~/run/bnet),)
	cp -f $(BNETTEST_LIB) ~/run/bnet
	cp -f $(DABCDLLPATH)/*.so ~/run/bnet
	cp -f $(BNETBINPATH)/* ~/run/bnet
endif
	@echo "$@  done"

clean:
	@rm -f $(BNETTEST_O) $(BNETTEST_DEP)
	@$(CleanLib) $(BNETTEST_LIBNAME) $(CURDIR)


include $(DABCSYSCORE)/config/Makefile.rules
