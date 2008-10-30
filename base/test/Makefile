include $(DABCSYS)/config/Makefile.config

CORETEST_S      = coretest.$(SrcSuf)
CORETEST_O      = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(CORETEST_S))
CORETEST_D      = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(CORETEST_S))
CORETEST_E      = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%, $(CORETEST_S))

SOCKETEST_S     = socketest.$(SrcSuf)
SOCKETEST_O     = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(SOCKETEST_S))
SOCKETEST_D     = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(SOCKETEST_S))
SOCKETEST_E     = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%, $(SOCKETEST_S))

ALLDEPENDENC += $(CORETEST_D) $(SOCKETEST_D)

tests::  $(CORETEST_E) $(SOCKETEST_E)

$(CORETEST_E):  $(CORETEST_O)
	$(LD) $(LDFLAGS) $(CORETEST_O) $(LIBS_CORESET) $(OutPutOpt) $(CORETEST_E)
	@echo "$@  done"

$(SOCKETEST_E):  $(SOCKETEST_O)
	$(LD) $(LDFLAGS) $(SOCKETEST_O) $(LIBS_CORESET) $(OutPutOpt) $(SOCKETEST_E)
	@echo "$@  done"

clean::
	@rm -f $(CORETEST_O) $(CORETEST_E) $(CORETEST_D)
	@rm -f $(SOCKETEST_O) $(SOCKETEST_E) $(SOCKETEST_D)


include $(DABCSYS)/config/Makefile.rules
