## normally should be like this for every plugin
## one can extract complete plugin subdirectory and compile it independent from rest

include $(DABCSYS)/config/Makefile.config

ifdef DABCMAINMAKE
MBSDIR = plugins/mbs
else
MBSDIR = .
INCLUDES += $(MBSDIR)
endif

MBSDIRI         = $(MBSDIR)/mbs
MBSDIRS         = $(MBSDIR)/src
MBSAPIDIR       = $(MBSDIR)/evapi
MBSFAPIDIR      = $(MBSDIR)/fileapi
MBSTESTDIR      = $(MBSDIR)/test

DABCMBS_LIBNAME  = $(LIB_PREFIX)DabcMbs
DABCMBS_LIB      = $(TGTDLLPATH)/$(DABCMBS_LIBNAME).$(DllSuf)

MBSAPI_DEFINITIONS = Linux

MBSFAPI_DEFINITIONS = Linux FILEONLY

MBS_H           = $(wildcard $(MBSDIRI)/*.$(HedSuf))
MBS_S           = $(wildcard $(MBSDIRS)/*.$(SrcSuf))
MBS_O           = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(MBS_S))
MBS_D           = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(MBS_S))

MBSAPI_H        = $(wildcard $(MBSAPIDIR)/*.$(HedSuf))
MBSAPI_CS       = $(wildcard $(MBSAPIDIR)/*.$(CSuf))
MBSAPI_CO       = $(patsubst %.$(CSuf), $(BLD_DIR)/%.$(ObjSuf), $(MBSAPI_CS))
MBSAPI_CD       = $(patsubst %.$(CSuf), $(BLD_DIR)/%.$(DepSuf), $(MBSAPI_CS))

MBSFAPI_LINKDEF = $(MBSFAPIDIR)/LinkDef.h
MBSFAPI_PACKAGE = $(MBSFAPIDIR)/lmdfile.tar.gz
MBSFAPI_TESTS = $(MBSFAPIDIR)/test.cxx

MBSFAPI_CS      = $(wildcard $(MBSFAPIDIR)/*.$(CSuf))
MBSFAPI_CH      = $(filter-out $(MBSFAPI_LINKDEF), $(wildcard $(MBSFAPIDIR)/*.$(HedSuf)))
MBSFAPI_S       = $(filter-out $(MBSFAPI_TESTS), $(wildcard $(MBSFAPIDIR)/*.$(SrcSuf)))
MBSFAPI_H       = $(MBSDIRI)/LmdTypeDefs.h $(MBSDIRI)/LmdFile.h 
MBSFAPI_CO      = $(patsubst %.$(CSuf), $(BLD_DIR)/%.$(ObjSuf), $(MBSFAPI_CS))
MBSFAPI_O       = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(MBSFAPI_S))
MBSFAPI_D       = $(patsubst %.$(ObjSuf), %.$(DepSuf), $(MBSFAPI_O))
MBSFAPI_CD      = $(patsubst %.$(ObjSuf), %.$(DepSuf), $(MBSFAPI_CO))
MBSFAPI_DISTR   = $(MBSFAPI_TESTS) $(MBSFAPI_CS) $(MBSFAPI_CH) $(MBSFAPI_S) \
                  $(MBSFAPI_LINKDEF) $(MBSFAPIDIR)/test.C $(MBSFAPIDIR)/readme.txt $(MBSFAPIDIR)/Makefile


# used in the main Makefile

ALLHDRS         += $(patsubst $(MBSDIR)/%.h, $(DABCINCPATH)/%.h, $(MBS_H))
ALLDEPENDENC    += $(MBS_D) $(MBSAPI_CD) $(MBSFAPI_D) $(MBSFAPI_CD)
TEST_DIRS       += $(MBSTESTDIR)

CREATE_DIRS     += $(TGTDLLPATH)

LIBS_PLUGINS    += $(DABCMBS_LIB)
#LIBS_EXTRA      += -L/my/dir abc.so 

libs:: $(DABCMBS_LIB) $(MBSFAPI_PACKAGE)

##### local rules #####

$(DABCINCPATH)/%.h: $(MBSDIR)/%.h
	@echo "Header: $@" 
	@cp -f $< $@

$(DABCMBS_LIB):   $(MBS_O) $(MBSAPI_CO) $(MBSFAPI_O) $(MBSFAPI_CO)
	@$(MakeLib) $(DABCMBS_LIBNAME) "$(MBS_O) $(MBSAPI_CO) $(MBSFAPI_O) $(MBSFAPI_CO)" $(TGTDLLPATH) "-lrt"

$(MBSFAPI_PACKAGE) : $(MBSFAPI_H) $(MBSFAPI_DISTR) 
	@rm -f $(MBSFAPI_PACKAGE)
	@cd $(MBSDIRI); tar cf ../fileapi/test.tar $(MBSFAPI_H:$(MBSDIRI)/%=%) 
	@cd $(MBSFAPIDIR); tar rf test.tar $(MBSFAPI_DISTR:$(MBSFAPIDIR)/%=%); gzip test.tar  
	@mv -f $(MBSFAPIDIR)/test.tar.gz $(MBSFAPI_PACKAGE)
	@echo "$(MBSFAPI_PACKAGE) produced!"

########### extra rules #############
$(MBS_O) $(MBS_D) : DEFINITIONS += $(MBSAPI_DEFINITIONS) 
$(MBSAPI_CO) $(MBSAPI_CD): C_DEFINITIONS += $(MBSAPI_DEFINITIONS)
$(MBSFAPI_O) $(MBSFAPI_D): DEFINITIONS += $(MBSFAPI_DEFINITIONS)
$(MBSFAPI_CO) $(MBSFAPI_CD): C_DEFINITIONS += $(MBSFAPI_DEFINITIONS)
$(MBSFAPI_O): INCLUDES += $(MBSDIRI)

include $(DABCSYS)/config/Makefile.rules