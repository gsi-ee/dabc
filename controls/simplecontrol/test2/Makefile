include $(DABCSYSCORE)/config/Makefile.config

TEST2_EXENAME     = test2

## must be similar for every module

TEST2_EXEO        = $(TEST2_EXENAME).$(ObjSuf)
TEST2_EXES        = $(TEST2_EXENAME).$(SrcSuf)
TEST2_EXE         = $(TEST2_EXENAME)$(ExeSuf)

TEST2_DEP        =  $(TEST2_EXEO:.$(ObjSuf)=.$(DepSuf))

ALLDEPENDENC    += $(TEST2_DEP)

all:  $(TEST2_EXE)

clean:
	@rm -f $(TEST2_EXEO) $(TEST2_EXE) $(TEST2_DEP)

$(TEST2_EXE): $(TEST2_EXEO) $(wildcard $(DABCDLLPATH)/*.$(DllSuf))
	$(LD) $(LDFLAGS) $(TEST2_EXEO) $(LIBS_FULLSET) $(OutPutOpt) $(TEST2_EXE)
ifneq ($(wildcard ~/run/test2),)
	cp -f $(TEST2_EXE) *.sh *.txt $(DABCDLLPATH)/*.$(DllSuf) ~/run/test2
endif
	@echo "$@  done"

include $(DABCSYSCORE)/config/Makefile.rules
