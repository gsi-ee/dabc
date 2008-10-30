include $(DABCSYSCORE)/config/Makefile.config

CORETEST_S      = coretest.cxx
CORETEST_O      = $(CORETEST_S:.$(SrcSuf)=.$(ObjSuf))
CORETEST_DEP    = $(CORETEST_O:.$(ObjSuf)=.$(DepSuf))
CORETEST_E      = coretest

SOCKETEST_S     = socketest.cxx
SOCKETEST_O     = $(SOCKETEST_S:.$(SrcSuf)=.$(ObjSuf))
SOCKETEST_DEP   = $(SOCKETEST_O:.$(ObjSuf)=.$(DepSuf))
SOCKETEST_E     = socketest


ALLDEPENDENC += $(CORETEST_DEP) $(SOCKETEST_DEP)

all:  $(CORETEST_E) $(SOCKETEST_E)

$(CORETEST_E):  $(CORETEST_O) $(DABCDLLPATH)/libDabcCore.so
	$(LD) $(LDFLAGS) $(CORETEST_O) $(LIBS_CORESET) $(OutPutOpt) $(CORETEST_E)
ifneq ($(wildcard ~/run/core),)
	cp -f $(CORETEST_E) $(DABCDLLPATH)/libDabcCore.so ~/run/core
endif
	@echo "$@  done"

$(SOCKETEST_E):  $(SOCKETEST_O) $(DABCDLLPATH)/libDabcCore.so
	$(LD) $(LDFLAGS) $(SOCKETEST_O) $(LIBS_CORESET) $(OutPutOpt) $(SOCKETEST_E)
ifneq ($(wildcard ~/run/core),)
	cp -f $(SOCKETEST_E) $(DABCDLLPATH)/libDabcCore.so ~/run/core
endif
	@echo "$@  done"

clean:
	@rm -f $(CORETEST_O) $(CORETEST_E) $(CORETEST_DEP)
	@rm -f $(SOCKETEST_O) $(SOCKETEST_E) $(SOCKETEST_DEP)

include $(DABCSYSCORE)/config/Makefile.rules
