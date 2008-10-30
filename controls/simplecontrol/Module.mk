MGMNTDIR         = mgmnt
MGMNTDIRI        = $(MGMNTDIR)/inc
MGMNTDIRS        = $(MGMNTDIR)/src
MGMNTTEST1DIR    = $(MGMNTDIR)/test1
MGMNTTEST2DIR    = $(MGMNTDIR)/test2

MGMNT_NOTLIBF     =

DABCMGMNT_LIBNAME  = $(LIB_PREFIX)DabcMgmnt
DABCMGMNT_LIB      = $(DABCDLLPATH)/$(DABCMGMNT_LIBNAME).$(DllSuf)

## must be similar for every module

MGMNT_H           = $(wildcard $(MGMNTDIRI)/*.$(HedSuf))
MGMNT_S           = $(wildcard $(MGMNTDIRS)/*.$(SrcSuf))
MGMNT_O           = $(MGMNT_S:.$(SrcSuf)=.$(ObjSuf))
MGMNT_DEP         = $(MGMNT_O:.$(ObjSuf)=.$(DepSuf))

# used in the main Makefile

ALLHDRS +=  $(patsubst $(MGMNTDIRI)/%.h, $(DABCINCPATH)/%.h, $(MGMNT_H))

BUILDDABCLIBS += $(DABCMGMNT_LIB)

ALLDEPENDENC       += $(MGMNT_DEP) 

DISTRFILES         += $(MGMNT_S) $(MGMNT_H)
DISTRFILES         += $(MGMNTTEST1DIR)/Makefile $(MGMNTTEST1DIR)/*.cxx $(MGMNTTEST1DIR)/*.txt $(MGMNTTEST1DIR)/*.sh
DISTRFILES         += $(MGMNTTEST2DIR)/Makefile $(MGMNTTEST2DIR)/*.cxx $(MGMNTTEST2DIR)/*.txt $(MGMNTTEST2DIR)/*.sh

##### local rules #####

$(DABCINCPATH)/%.h: $(MGMNTDIRI)/%.h
	@cp -f $< $@

$(DABCMGMNT_LIB):   $(MGMNT_O)
	@$(MakeLib) $(DABCMGMNT_LIBNAME) "$(MGMNT_O)" $(DABCDLLPATH)

all-mgmnt:  $(DABCMGMNT_LIB) mgmnt-test

clean-mgmnt: mgmnt-clean-test
	@$(CleanLib) $(DABCMGMNT_LIBNAME) $(DABCDLLPATH)
	@rm -f $(MGMNT_O) $(MGMNT_DEP)

mgmnt-test: $(DABCMBS_LIB) $(DABCCORE_LIB)
	@export DABCSYSCORE=$(CURDIR); cd $(MGMNTTEST1DIR); make all
	@export DABCSYSCORE=$(CURDIR); cd $(MGMNTTEST2DIR); make all

mgmnt-clean-test:
	@export DABCSYSCORE=$(CURDIR); cd $(MGMNTTEST1DIR); make clean
	@export DABCSYSCORE=$(CURDIR); cd $(MGMNTTEST2DIR); make clean

clean:: clean-mgmnt

all:: all-mgmnt
