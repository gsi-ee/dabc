include $(DABCSYS)/config/Makefile.config

ifdef DABCMAINMAKE
CORETESTDIR = applications/core-test/
else
CORETESTDIR = 
endif

CORETEST_LIBNAME     = $(LIB_PREFIX)DabcCoreTest
CORETEST_LIB         = $(CORETESTDIR)$(CORETEST_LIBNAME).$(DllSuf)

CORETEST_S      = $(CORETESTDIR)core-test.$(SrcSuf)
CORETEST_O      = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(CORETEST_S))
CORETEST_D      = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(CORETEST_S))

ALLDEPENDENC += $(CORETEST_D)

libs::  $(CORETEST_LIB)

clean::
	@$(RM) $(CORETEST_LIB)

$(CORETEST_LIB):  $(CORETEST_O)
	@$(MakeLib) $(CORETEST_LIBNAME) "$(CORETEST_O)" $(CORETESTDIR)

include $(DABCSYS)/config/Makefile.rules
