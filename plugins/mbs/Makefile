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
MBSFAPIDIR      = $(MBSDIR)/fileapi

DABCMBS_LIBNAME  = $(LIB_PREFIX)DabcMbs
DABCMBS_LIB      = $(TGTDLLPATH)/$(DABCMBS_LIBNAME).$(DllSuf)

MBSFAPI_DEFINITIONS = Linux FILEONLY _LARGEFILE64_SOURCE WITH_DABC

MBS_H           = $(wildcard $(MBSDIRI)/*.$(HedSuf))
MBS_S           = $(wildcard $(MBSDIRS)/*.$(SrcSuf))
MBS_O           = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(MBS_S))
MBS_D           = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(MBS_S))

MBSFAPI_PACKAGE = $(MBSFAPIDIR)/lmdfile.tar.gz

MBSFAPI_CS      = $(MBSFAPIDIR)/fLmd.c
MBSFAPI_CH      = $(MBSFAPIDIR)/fLmd.h $(MBSFAPIDIR)/sMbs.h 
MBSFAPI_CO      = $(patsubst %.$(CSuf), $(BLD_DIR)/%.$(ObjSuf), $(MBSFAPI_CS))
MBSFAPI_CD      = $(patsubst %.$(ObjSuf), %.$(DepSuf), $(MBSFAPI_CO))

MBSFAPI_S       = $(MBSDIRS)/MbsTypeDefs.cxx $(MBSDIRS)/LmdFile.cxx 
MBSFAPI_H       = $(MBSDIRI)/LmdTypeDefs.h $(MBSDIRI)/MbsTypeDefs.h $(MBSDIRI)/LmdFile.h 
MBSFAPI_O       = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(MBSFAPI_S))
MBSFAPI_D       = $(patsubst %.$(ObjSuf), %.$(DepSuf), $(MBSFAPI_O))

MBSFAPI_DISTR   = $(MBSFAPI_CS) $(MBSFAPI_CH) \
                  $(MBSFAPIDIR)/LinkDef.h $(MBSFAPIDIR)/readme.txt $(MBSFAPIDIR)/Makefile \
                  $(MBSFAPIDIR)/test.cxx $(MBSFAPIDIR)/test.C  

DABCMBS_O       = $(MBS_O) $(MBSFAPI_CO)
DABCMBS_D       = $(MBS_D) $(MBSFAPI_CD) $(MBSFAPI_D)

# used in the main Makefile

ALLHDRS           += $(patsubst $(MBSDIR)/%.h, $(DABCINCPATH)/%.h, $(MBS_H))
ALLDEPENDENC      += $(DABCMBS_D)

libs:: $(DABCMBS_LIB) $(MBSFAPI_PACKAGE)

##### local rules #####

$(DABCINCPATH)/%.h: $(MBSDIR)/%.h
	@echo "Header: $@" 
	@cp -f $< $@


$(DABCMBS_LIB): LDFLAGS += -lrt

$(DABCMBS_LIB):   $(DABCMBS_O)
	@$(MakeLib) $(DABCMBS_LIBNAME) "$(DABCMBS_O)" $(TGTDLLPATH) 

$(MBSFAPI_PACKAGE) : $(MBSFAPI_H) $(MBSFAPI_S) $(MBSFAPI_DISTR) 
	@rm -f $(MBSFAPI_PACKAGE)
	@cd $(MBSDIRI); tar cf ../fileapi/test.tar $(MBSFAPI_H:$(MBSDIRI)/%=%) 
	@cd $(MBSDIRS); tar rf ../fileapi/test.tar $(MBSFAPI_S:$(MBSDIRS)/%=%) 
	@cd $(MBSFAPIDIR); tar rf test.tar $(MBSFAPI_DISTR:$(MBSFAPIDIR)/%=%); gzip test.tar  
	@mv -f $(MBSFAPIDIR)/test.tar.gz $(MBSFAPI_PACKAGE)
	@echo "$(MBSFAPI_PACKAGE) produced!"

clean::
	rm -f $(MBSFAPI_PACKAGE)

########### extra rules #############
$(MBS_O) $(MBS_D) : DEFINITIONS += $(MBSAPI_DEFINITIONS) 
$(MBSFAPI_O) $(MBSFAPI_D): DEFINITIONS += $(MBSFAPI_DEFINITIONS)
$(MBSFAPI_CO) $(MBSFAPI_CD): C_DEFINITIONS += $(MBSFAPI_DEFINITIONS)
$(MBSFAPI_O) $(MBSFAPI_D) $(MBSFAPI_CD): INCLUDES += $(MBSDIRI) $(MBSFAPIDIR)

include $(DABCSYS)/config/Makefile.rules
