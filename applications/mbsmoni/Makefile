include $(DABCSYS)/config/Makefile.config

ifdef DABCMAINMAKE
MBSMONIDIR = applications/mbsmoni
else
MBSMONIDIR = .
INCLUDES += $(MBSMONIDIR)
endif

MBSMONI_EXENAME  = mMbsDimStatus
MBSMONI_EXE      = $(MBSMONIDIR)/$(MBSMONI_EXENAME)

MBSMONI_PACKAGE = $(MBSMONIDIR)/mbsmoni.tar.gz

MBSMONI_H         = $(wildcard $(MBSMONIDIR)/*.$(HedSuf))
MBSMONI_C         = $(wildcard $(MBSMONIDIR)/*.$(CSuf))
MBSMONI_CO        = $(patsubst %.$(CSuf), $(BLD_DIR)/%.$(ObjSuf), $(MBSMONI_C))
MBSMONI_CD        = $(patsubst %.$(CSuf), $(BLD_DIR)/%.$(DepSuf), $(MBSMONI_C))
MBSMONI_CX        = $(wildcard $(MBSMONIDIR)/*.$(SrcSuf))
MBSMONI_CXO       = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(MBSMONI_CX))
MBSMONI_CXD       = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(MBSMONI_CX))

MBSMONI_DEFINITIONS = Linux 


# used in the main Makefile

ALLDEPENDENC += $(MBSMONI_CD) $(MBSMONI_CXD)

exes:: $(MBSMONI_EXE)

clean::
	rm -f $(MBSMONI_EXE)

$(MBSMONI_EXE):   $(MBSMONI_CO) $(MBSMONI_CXO) $(DABCDLLPATH)/libdim.$(DllSuf)
	$(LD) $(LDFLAGS) $(MBSMONI_CO) $(MBSMONI_CXO) -lrt -L$(DABCDLLPATH) -ldim $(OutPutOpt) $(MBSMONI_EXE)

$(MBSMONI_CXO) : DEFINITIONS += $(MBSMONI_DEFINITIONS) 

$(MBSMONI_CO) : C_DEFINITIONS += $(MBSMONI_DEFINITIONS)

$(MBSMONI_CXO) : INCLUDES += $(DIMDIR)/dim

include $(DABCSYS)/config/Makefile.rules
