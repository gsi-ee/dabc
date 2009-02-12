include $(DABCSYS)/config/Makefile.config

ifdef DABCMAINMAKE
NETTESTDIR = applications/net-test/
else
NETTESTDIR = 
endif

NETTEST_LIBNAME   = $(LIB_PREFIX)DabcNetTest
NETTEST_LIB       = $(NETTESTDIR)$(NETTEST_LIBNAME).$(DllSuf)

NETTEST_S         = $(NETTESTDIR)net-test.$(SrcSuf)
NETTEST_O         = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(NETTEST_S))
NETTEST_D         = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(NETTEST_S))

ALLDEPENDENC += $(NETTEST_D)

libs::  $(NETTEST_LIB)

clean::
	rm -f $(NETTEST_LIB)

$(NETTEST_LIB):  $(NETTEST_O)
	@$(MakeLib) $(NETTEST_LIBNAME) "$(NETTEST_O)" $(NETTESTDIR)

include $(DABCSYS)/config/Makefile.rules
