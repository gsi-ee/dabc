## normally should be like this for every plugin
## one can extract complete plugin subdirectory and compile it independent from rest

ifndef noltsm

include $(DABCSYS)/config/Makefile.config

ifdef DABC_LTSM

# TODO: install headers at system include location

# all definitions checks are in Makefile.config now
#LTSMAPI = /home/adamczew/git/ltsm_jb/src/lib
#TSMAPI  =/opt/tivoli/tsm/client/api/bin64/sample/



ifdef DABCMAINMAKE
LTSMDIR = plugins/ltsm
else
LTSMDIR = .
endif

#DABC_TSM_LIBDIR	 = /usr/lib64/
#TSM_LINKLIBS     = -lApiTSM64 

LTSMDIRI         = $(LTSMDIR)/ltsm
LTSMDIRS         = $(LTSMDIR)/src

DABCLTSM_LIBNAME  = $(LIB_PREFIX)DabcLtsm
DABCLTSM_LIB      = $(TGTDLLPATH)/$(DABCLTSM_LIBNAME).$(DllSuf)


LTSM_H           = $(wildcard $(LTSMDIRI)/*.$(HedSuf))
LTSM_S           = $(wildcard $(LTSMDIRS)/*.$(SrcSuf))
LTSM_O           = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(LTSM_S))
LTSM_D           = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(LTSM_S))

DABCLTSM_O       = $(LTSM_O)
DABCLTSM_O       += $(wildcard $(LTSMAPI)/*.a)



 
$(LTSM_O) $(LTSM_D) : INCLUDES += $(LTSMAPI) $(TSMAPI)


DABCLTSM_D       = $(LTSM_D)






# used in the main Makefile

ALLHDRS           += $(patsubst $(LTSMDIR)/%.h, $(DABCINCPATH)/%.h, $(LTSM_H))
ALLDEPENDENC      += $(DABCLTSM_D)

libs:: $(DABCLTSM_LIB)

##### local rules #####

$(DABCINCPATH)/%.h: $(LTSMDIR)/%.h
	@echo "Header: $@" 
	@cp -f $< $@

$(DABCLTSM_LIB):   $(DABCLTSM_O)
	@$(MakeLib) $(DABCLTSM_LIBNAME) "$(DABCLTSM_O)" $(TGTDLLPATH) "$(call lddiropt,$(TSM_LIBDIR)) $(TSM_LINKLIBS)"

clean::
#	rm -f $(LTSMFAPI_PACKAGE)

########### extra rules #############

$(LTSM_CO) $(LTSM_CD): C_DEFINITIONS += Linux

$(LTSM_O) : CXXFLAGS += -Wno-unused-variable

ifdef DABC_64BIT 
$(LTSM_CO) $(LTSM_CD): C_DEFINITIONS += SYSTEM64 
endif

include $(DABCSYS)/config/Makefile.rules

endif
endif
