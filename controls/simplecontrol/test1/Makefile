include $(DABCSYSCORE)/config/Makefile.config

TEST1_EXENAME     = test1

## must be similar for every module

TEST1_EXEO        = $(TEST1_EXENAME).$(ObjSuf)
TEST1_EXES        = $(TEST1_EXENAME).$(SrcSuf)
TEST1_EXE         = $(TEST1_EXENAME)$(ExeSuf)

TEST1_DEP         =  $(TEST1_EXEO:.$(ObjSuf)=.$(DepSuf))

ALLDEPENDENC += $(TEST1_DEP)

all:  $(TEST1_EXE)

clean: 
	@rm -f $(TEST1_DEP) $(TEST1_EXEO) $(TEST1_EXE)

$(TEST1_EXE):  $(TEST1_EXEO) $(wildcard $(DABCDLLPATH)/*.$(DllSuf))
	$(LD) $(LDFLAGS) $(TEST1_EXEO) $(LIBS_FULLSET) $(OutPutOpt) $(TEST1_EXE)
ifneq ($(wildcard ~/run/test1),)
	cp -f $(TEST1_EXE) *.sh *.txt $(DABCDLLPATH)/*.$(DllSuf) ~/run/test1
endif
	@echo "$@  done"

include $(DABCSYSCORE)/config/Makefile.rules
