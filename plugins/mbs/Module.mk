## normally should be like this for every module, but can be specific

MBSDIR          = mbs
MBSDIRI         = $(MBSDIR)/inc
MBSDIRS         = $(MBSDIR)/src
MBSAPIDIR       = $(MBSDIR)/evapi
MBSFAPIDIR      = $(MBSDIR)/fileapi
MBSINCPATH      = include/mbs
MBSTESTDIR      = $(MBSDIR)/test

MBS_NOTLIBF     =

DABCMBS_LIBNAME  = $(LIB_PREFIX)DabcMbs
DABCMBS_LIB      = $(DABCDLLPATH)/$(DABCMBS_LIBNAME).$(DllSuf)

MBSAPI_DEFINITIONS = Linux

MBSFAPI_DEFINITIONS = Linux FILEONLY

MBSFAPI_TESTS = $(MBSFAPIDIR)/test.cxx
MBSFAPI_LINKDEF = $(MBSFAPIDIR)/LinkDef.h
MBSFAPI_PACKAGE = $(MBSFAPIDIR)/lmdfile.tar.gz

## must be similar for every module

MBS_H           = $(filter-out $(MBS_NOTLIBF), $(wildcard $(MBSDIRI)/*.$(HedSuf)))
MBS_S           = $(filter-out $(MBS_NOTLIBF), $(wildcard $(MBSDIRS)/*.$(SrcSuf)))
MBS_O           = $(MBS_S:.$(SrcSuf)=.$(ObjSuf))
MBS_DEP         = $(MBS_O:.$(ObjSuf)=.$(DepSuf))

MBSAPI_H        = $(wildcard $(MBSAPIDIR)/*.$(HedSuf))
MBSAPI_S        = $(wildcard $(MBSAPIDIR)/*.$(CSuf))
MBSAPI_O        = $(MBSAPI_S:.$(CSuf)=.$(ObjSuf))
MBSAPI_DEP      = $(MBSAPI_O:.$(ObjSuf)=.$(DepSuf))

MBSFAPI_CS      = $(wildcard $(MBSFAPIDIR)/*.$(CSuf))
MBSFAPI_S       = $(filter-out $(MBSFAPI_TESTS), $(wildcard $(MBSFAPIDIR)/*.$(SrcSuf)))
MBSFAPI_H       = $(MBSFAPI_S:.$(SrcSuf)=.$(HedSuf))
MBSFAPI_O       = $(MBSFAPI_CS:.$(CSuf)=.$(ObjSuf)) $(MBSFAPI_S:.$(SrcSuf)=.$(ObjSuf))
MBSFAPI_DEP     = $(MBSFAPI_O:.$(ObjSuf)=.$(DepSuf))
MBSFAPI_DISTR   = $(MBSFAPI_S) $(MBSFAPI_CS) $(MBSFAPIDIR)/Makefile $(MBSFAPIDIR)/*.$(HedSuf) $(MBSFAPIDIR)/*.txt $(MBSFAPIDIR)/*.C $(MBSFAPI_TESTS)
# used in the main Makefile

ALLHDRS +=  $(patsubst $(MBSDIRI)/%.h, $(MBSINCPATH)/%.h, $(MBS_H))
ALLHDRS +=  $(patsubst $(MBSFAPIDIR)/%.h, $(MBSINCPATH)/%.h, $(MBSFAPI_H))


BUILDDABCLIBS      += $(DABCMBS_LIB)

ALLDEPENDENC       += $(MBS_DEP) $(MBSAPI_DEP) $(MBSFAPI_DEP)

DISTRFILES         += $(MBS_S) $(MBS_H) $(MBS_NOTLIBF) 
DISTRFILES         += $(MBSAPI_H) $(MBSAPI_S)
DISTRFILES         += $(MBSFAPI_DISTR)
DISTRFILES         += $(MBSTESTDIR)/*.cxx $(MBSTESTDIR)/Makefile

##### local rules #####

build-init::
	@(if [ ! -f $(MBSINCPATH) ] ; then mkdir -p $(MBSINCPATH); fi)

$(MBSINCPATH)/%.h: $(MBSDIRI)/%.h
	@cp -f $< $@

$(MBSINCPATH)/%.h: $(MBSFAPIDIR)/%.h
	@cp -f $< $@

$(DABCMBS_LIB):   $(MBS_O) $(MBSAPI_O) $(MBSFAPI_O)
	@$(MakeLib) $(DABCMBS_LIBNAME) "$(MBS_O) $(MBSAPI_O) $(MBSFAPI_O)" $(DABCDLLPATH) "-lrt"

$(MBSFAPI_PACKAGE) : $(MBSFAPI_DISTR)
	@rm -f $(MBSFAPI_PACKAGE)
	@cd $(MBSFAPIDIR); tar chf test.tar $(MBSFAPI_DISTR:$(MBSFAPIDIR)/%=%); gzip test.tar; 
	@mv -f $(MBSFAPIDIR)/test.tar.gz $(MBSFAPI_PACKAGE)
	@echo "$(MBSFAPI_PACKAGE) produced!"

all-mbs:  $(DABCMBS_LIB) $(BUILDDABCLIBS) $(MBSFAPI_PACKAGE) mbs-test 

clean-mbs: clean-mbs-test
	@rm -f $(MBS_O) $(MBSAPI_O)
	@$(CleanLib) $(DABCMBS_LIBNAME) $(DABCDLLPATH)
	@rm -f $(MBS_DEP) $(MBSAPI_DEP)
	@rm -f $(MBSFAPI_DEP) $(MBSFAPI_O)

all:: all-mbs

clean:: clean-mbs

mbs-test: $(DABCMBS_LIB) $(DABCCORE_LIB)
	@export DABCSYSCORE=$(CURDIR); cd $(MBSTESTDIR); make all

clean-mbs-test:
	@export DABCSYSCORE=$(CURDIR); cd $(MBSTESTDIR); make clean

########### extra rules #############
$(MBS_O) $(MBS_DEP) $(MBSAPI_O) $(MBSAPI_DEP): DEFINITIONS += $(MBSAPI_DEFINITIONS:%=-D%)
$(MBSFAPI_O) $(MBSFAPI_DEP): DEFINITIONS += $(MBSFAPI_DEFINITIONS:%=-D%)
