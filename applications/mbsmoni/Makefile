include $(DABCSYS)/config/Makefile.config

MBSMONI=1

ifdef DABCMAINMAKE
MBSMONIDIR = applications/mbsmoni
else
MBSMONIDIR = .
INCLUDES += $(MBSMONIDIR)
endif

MBSMONI_LIBNAME  = $(LIB_PREFIX)MbsDimStatus
MBSMONI_LIB      = $(MBSMONIDIR)/$(MBSMONI_LIBNAME).$(DllSuf)

MBSMONI_PACKAGE = $(MBSMONIDIR)/mbsmoni.tar.gz

ifdef MBSMONI

MBSMONI_H        = $(wildcard $(MBSMONIDIR)/*.$(HedSuf))
MBSMONI_C       = $(wildcard $(MBSMONIDIR)/*.$(CSuf))
MBSMONI_CO       = $(patsubst %.$(CSuf), $(BLD_DIR)/%.$(ObjSuf), $(MBSMONI_C))
MBSMONI_CD       = $(patsubst %.$(CSuf), $(BLD_DIR)/%.$(DepSuf), $(MBSMONI_C))
MBSMONI_CX        = $(wildcard $(MBSMONIDIR)/*.$(SrcSuf))
MBSMONI_CXO       = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(MBSMONI_CX))
MBSMONI_CXD       = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(MBSMONI_CX))

MBSMONI_DEFINITIONS = Linux 
else
MBSMONI_EXCLUDE +=  $(wildcard $(MBSMONIDIR)/*.$(HedSuf)) \
                    $(wildcard $(MBSMONIDIR)/*.$(SrcSuf)) \
                    $(wildcard $(MBSMONIDIR)/*.$(CSuf)) \
endif

# used in the main Makefile

ALLDEPENDENC += $(MBSMONI_CD) $(MBSMONI_CXD)
ALLHDRS      += $(patsubst $(MBSMONIDIR)/%.h, $(DABCINCPATH)/%.h, $(MBSMONI_H))

libs:: $(MBSMONI_LIB)
clean::
	rm -f $(MBSMONI_LIB)

$(MBSMONI_LIB):   $(MBSMONI_CO) $(MBSMONI_CXO)
	@$(MakeLib) $(MBSMONI_LIBNAME) "$(DABCMBS_CO) $(DABCMBS_CXO)" $(MBSMONIDIR)


include $(DABCSYS)/config/Makefile.rules