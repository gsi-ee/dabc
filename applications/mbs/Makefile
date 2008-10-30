include $(DABCSYSCORE)/config/Makefile.config

# DOOPTIMIZATION = false

BNETBINPATH           = $(DABCSYSCORE)/bnet/bin

BNETTEST_EXENAME     = genmbsfiles
BNETTEST_NOTLIBF     = 
BNETTEST_LIBNAME     = $(LIB_PREFIX)BnetWorker
BNETTEST_LIB         = $(BNETTEST_LIBNAME).$(DllSuf)

BNETTEST_EXEO        = $(BNETTEST_EXENAME).$(ObjSuf)
BNETTEST_EXES        = $(BNETTEST_EXENAME).$(SrcSuf)
BNETTEST_EXE         = $(BNETTEST_EXENAME)$(ExeSuf)   

BNETTEST_H           = $(filter-out $(BNETTEST_EXEH) $(BNETTEST_NOTLIBF), $(wildcard *.$(HedSuf)))
BNETTEST_S           = $(filter-out $(BNETTEST_EXES) $(BNETTEST_NOTLIBF), $(wildcard *.$(SrcSuf)))
BNETTEST_O           = $(BNETTEST_S:.$(SrcSuf)=.$(ObjSuf))

BNETTEST_DEP         =  $(BNETTEST_O:.$(ObjSuf)=.$(DepSuf))
BNETTEST_EDEP        =  $(BNETTEST_EXEO:.$(ObjSuf)=.$(DepSuf))

ALLDEPENDENC        += $(BNETTEST_DEP) $(BNETTEST_EDEP)

all:  $(BNETTEST_LIB) $(BNETTEST_EXE) 

$(BNETTEST_LIB):   $(BNETTEST_O)
	@$(MakeLib) $(BNETTEST_LIBNAME) "$(BNETTEST_O)" $(CURDIR)

$(BNETTEST_EXE):  $(BNETTEST_EXEO) $(BNETTEST_LIB) $(wildcard $(BNETBINPATH)/*) $(wildcard $(DABCDLLPATH)/*.so) Makefile genmbsfiles.sh
	$(LD) $(LDFLAGS) $(BNETTEST_EXEO) $(LIBS_FULLSET) -L$(BNETBINPATH) -lDabcBnet -L. -lBnetWorker $(OutPutOpt) $(BNETTEST_EXE)
ifneq ($(wildcard ~/run/bnet/mbs),)
	cp -f $(BNETTEST_EXE) $(BNETTEST_LIB) $(DABCDLLPATH)/*.so $(BNETBINPATH)/* *.sh ~/run/bnet/mbs
endif


clean:
	@rm -f $(BNETTEST_EXEO) $(BNETTEST_EXE)
	@rm -f $(BNETTEST_O)
	@rm -f $(BNETTEST_EDEP) $(BNETTEST_DEP)
	@$(CleanLib) $(BNETTEST_LIBNAME) $(CURDIR)

include $(DABCSYSCORE)/config/Makefile.rules
