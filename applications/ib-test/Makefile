ifndef USEOFEDVERBS
ifneq ($(wildcard $(DABCSYS)/lib/libDabcVerbs.*),)
USEOFEDVERBS    = true
endif
endif


include $(DABCSYS)/config/Makefile.config

ifdef DABCMAINMAKE
IBTESTDIR = applications/ib-test/
else
IBTESTDIR = 
endif

EXTRAOPT = 

IBTEST_LIBNAME   = $(LIB_PREFIX)DabcIbTest
IBTEST_LIB       = $(IBTESTDIR)$(IBTEST_LIBNAME).$(DllSuf)

IBTEST_S         = $(wildcard $(IBTESTDIR)*.$(SrcSuf))
IBTEST_O         = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(IBTEST_S))
IBTEST_D         = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(IBTEST_S))

ALLDEPENDENC += $(IBTEST_D)


libs::  $(IBTEST_LIB)

ifeq ($(USEOFEDVERBS), true)
EXTRAOPT+="-L$(DABCSYS)/lib -lDabcVerbs"

libs::
	cd $(IBTESTDIR)tracert; $(MAKE)
ifdef DABCMAINMAKE
	cp -f $(IBTESTDIR)tracert/ibtracertm $(DABCBINPATH)
endif

clean::
	cd $(IBTESTDIR)tracert; $(MAKE) clean

endif


clean::
	rm -f $(IBTEST_LIB) $(IBTESTDIR)*.log

$(IBTEST_LIB):  $(IBTEST_O)
	@$(MakeLib) $(IBTEST_LIBNAME) "$(IBTEST_O)" $(IBTESTDIR) $(EXTRAOPT)

ifeq ($(USEOFEDVERBS),true)

$(IBTEST_O) : DEFINITIONS += WITH_VERBS

endif


include $(DABCSYS)/config/Makefile.rules
