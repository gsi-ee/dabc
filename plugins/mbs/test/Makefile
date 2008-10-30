include $(DABCSYSCORE)/config/Makefile.config

MBSTEST1_S      = mbstest.cxx
MBSTEST1_O      = $(MBSTEST1_S:.$(SrcSuf)=.$(ObjSuf))
MBSTEST1_DEP    = $(MBSTEST1_O:.$(ObjSuf)=.$(DepSuf))
MBSTEST1_E      = mbstest

MBSTEST2_S      = mbsservtest.cxx
MBSTEST2_O      = $(MBSTEST2_S:.$(SrcSuf)=.$(ObjSuf))
MBSTEST2_DEP    = $(MBSTEST2_O:.$(ObjSuf)=.$(DepSuf))
MBSTEST2_E      = mbsservtest

ALLDEPENDENC += $(MBSTEST1_DEP) $(MBSTEST2_DEP)

all:  $(MBSTEST1_E) $(MBSTEST2_E)

$(MBSTEST1_E):  $(MBSTEST1_O) $(wildcard $(DABCDLLPATH)/*.$(DllSuf))
	$(LD) $(LDFLAGS) $(MBSTEST1_O) $(LIBS_CORESET) -lDabcMbs $(OutPutOpt) $(MBSTEST1_E)
ifneq ($(wildcard ~/run/mbs),)
	cp -f $(MBSTEST1_E) ~/run/mbs
	cp -f $(DABCDLLPATH)/libDabcCore.so $(DABCDLLPATH)/libDabcMbs.so ~/run/mbs
endif
	@echo "$@  done"

$(MBSTEST2_E):  $(MBSTEST2_O) $(wildcard $(DABCDLLPATH)/*.$(DllSuf))
	$(LD) $(LDFLAGS) $(MBSTEST2_O) $(LIBS_CORESET) -lDabcMbs $(OutPutOpt) $(MBSTEST2_E)
ifneq ($(wildcard ~/run/mbs),)
	cp -f $(MBSTEST2_E) ~/run/mbs
	cp -f $(DABCDLLPATH)/libDabcCore.so $(DABCDLLPATH)/libDabcMbs.so ~/run/mbs
endif
	@echo "$@  done"

clean:
	@rm -f $(MBSTEST1_O) $(MBSTEST1_E) $(MBSTEST1_DEP)
	@rm -f $(MBSTEST2_O) $(MBSTEST2_E) $(MBSTEST2_DEP)

include $(DABCSYSCORE)/config/Makefile.rules

