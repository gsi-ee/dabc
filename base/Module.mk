## normally should be like this for every module, but can be specific

COREDIR          = core
COREDIRI         = $(COREDIR)/inc
COREDIRS         = $(COREDIR)/src
CORETESTDIR      = $(COREDIR)/test

CORE_NOTLIBF     =

DABCCORE_LIBNAME  = $(LIB_PREFIX)DabcCore
DABCCORE_LIB      = $(DABCDLLPATH)/$(DABCCORE_LIBNAME).$(DllSuf)

## must be similar for every module

CORE_H           = $(wildcard $(COREDIRI)/*.$(HedSuf))
CORE_S           = $(wildcard $(COREDIRS)/*.$(SrcSuf))
CORE_O           = $(CORE_S:.$(SrcSuf)=.$(ObjSuf))
CORE_DEP         = $(CORE_O:.$(ObjSuf)=.$(DepSuf))

# used in the main Makefile

ALLHDRS +=  $(patsubst $(COREDIRI)/%.h, $(DABCINCPATH)/%.h, $(CORE_H))

BUILDDABCLIBS += $(DABCCORE_LIB)

ALLDEPENDENC       += $(CORE_DEP) 

DISTRFILES         += $(CORE_S) $(CORE_H)
DISTRFILES         += $(CORETESTDIR)/Makefile  $(CORETESTDIR)/*.cxx

##### local rules #####

include/dabc/%.h: $(COREDIRI)/%.h
	@cp -f $< $@

$(DABCCORE_LIB):   $(CORE_O)
	@$(MakeLib) $(DABCCORE_LIBNAME) "$(CORE_O)" $(DABCDLLPATH)

all-core:  $(DABCCORE_LIB) core-test

clean-core: clean-core-test
	@$(CleanLib) $(DABCCORE_LIBNAME) $(DABCDLLPATH)
	@rm -f $(CORE_O) $(CORE_DEP)

core-test: $(DABCCORE_LIB)
	@export DABCSYSCORE=$(CURDIR); cd $(CORETESTDIR); make all

clean-core-test:
	@export DABCSYSCORE=$(CURDIR); cd $(CORETESTDIR); make clean

all:: all-core

clean:: clean-core  

