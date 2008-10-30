FASTMON_NAME        = FastMon
MODULE_NAME         = $(FASTMON_NAME)

## normally should be like this for every module, but can be specific

ifdef GO4PACKAGE
FASTMON_DIR         = $(GO4SYS)/$(FASTMON_NAME)
else
FASTMON_DIR         = .
endif

FASTMON_LINKDEF     = $(FASTMON_DIR)/Go4UserAnalysisLinkDef.$(HedSuf)
FASTMON_LIBNAME     = libGo4UserAnalysis
FASTMON_EXENAME     = MainUserAnalysis
FASTMON_ONLYMAP     = $(FASTMON_DIR)/.localmap
FASTMON_MAP         = $(FASTMON_DIR)/$(ROOTMAPNAME)

FASTMON_NOTLIBF     = 

## must be similar for every module

FASTMON_EXEO        = $(FASTMON_DIR)/$(FASTMON_EXENAME).$(ObjSuf)
#FASTMON_EXEH        = $(FASTMON_DIR)/$(FASTMON_EXENAME).$(HedSuf)
FASTMON_EXES        = $(FASTMON_DIR)/$(FASTMON_EXENAME).$(SrcSuf)
FASTMON_EXE         = $(FASTMON_DIR)/$(FASTMON_EXENAME)$(ExeSuf)   

FASTMON_DICT        = $(FASTMON_DIR)/$(DICT_PREFIX)$(FASTMON_NAME)
FASTMON_DH          = $(FASTMON_DICT).$(HedSuf)
FASTMON_DS          = $(FASTMON_DICT).$(SrcSuf)
FASTMON_DO          = $(FASTMON_DICT).$(ObjSuf)

FASTMON_H           = $(filter-out $(FASTMON_EXEH) $(FASTMON_NOTLIBF) $(FASTMON_DH) $(FASTMON_LINKDEF), $(wildcard $(FASTMON_DIR)/*.$(HedSuf)))
FASTMON_S           = $(filter-out $(FASTMON_EXES) $(FASTMON_NOTLIBF) $(FASTMON_DS), $(wildcard $(FASTMON_DIR)/*.$(SrcSuf)))

FASTMON_O           = $(FASTMON_S:.$(SrcSuf)=.$(ObjSuf))

FASTMON_DEP         =  $(FASTMON_O:.$(ObjSuf)=.$(DepSuf))
FASTMON_DDEP        =  $(FASTMON_DO:.$(ObjSuf)=.$(DepSuf))
FASTMON_EDEP        =  $(FASTMON_EXEO:.$(ObjSuf)=.$(DepSuf))

FASTMON_SLIB        =  $(FASTMON_DIR)/$(FASTMON_LIBNAME).$(DllSuf)
FASTMON_LIB         =  $(FASTMON_DIR)/$(FASTMON_LIBNAME).$(DllSuf).$(VERSSUF)

# used in the main Makefile

EXAMPDEPENDENCS    += $(FASTMON_DEP) $(FASTMON_DDEP) $(FASTMON_EDEP)

ifdef DOPACKAGE
DISTRFILES         += $(FASTMON_S) $(FASTMON_H) $(FASTMON_LINKDEF) $(FASTMON_EXES)
endif

##### local rules #####

$(FASTMON_EXE):      $(BUILDGO4LIBS) $(FASTMON_EXEO) $(FASTMON_LIB)
	$(LD) $(LDFLAGS) $(FASTMON_EXEO) $(LIBS_FULLSET) $(FASTMON_LIB) $(OutPutOpt) $(FASTMON_EXE)
	@echo "$@  done"

$(FASTMON_LIB):   $(FASTMON_O) $(FASTMON_DO)
	@$(MakeLib) $(FASTMON_LIBNAME) "$(FASTMON_O) $(FASTMON_DO)" $(FASTMON_DIR)

$(FASTMON_DS): $(FASTMON_H)  $(FASTMON_LINKDEF)
	@$(ROOTCINTGO4) $(FASTMON_H) $(FASTMON_LINKDEF)

$(FASTMON_ONLYMAP): $(FASTMON_LINKDEF) $(FASTMON_LIB)
	@rm -f $(FASTMON_ONLYMAP)
	@$(MakeMap) $(FASTMON_ONLYMAP) $(FASTMON_SLIB) $(FASTMON_LINKDEF) "$(ANAL_LIB_DEP)"

all-$(FASTMON_NAME):     $(FASTMON_LIB) $(FASTMON_EXE) map-$(FASTMON_NAME)

clean-obj-$(FASTMON_NAME):
	@rm -f $(FASTMON_O) $(FASTMON_DO)
	@$(CleanLib) $(FASTMON_LIBNAME) $(FASTMON_DIR)
	@rm -f $(FASTMON_EXEO) $(FASTMON_EXE)

clean-$(FASTMON_NAME): clean-obj-$(FASTMON_NAME)
	@rm -f $(FASTMON_DEP) $(FASTMON_DDEP) $(FASTMON_DS) $(FASTMON_DH)
	@rm -f $(FASTMON_EDEP)
	@rm -f $(FASTMON_ONLYMAP) $(FASTMON_MAP)

ifdef DOMAP
map-$(FASTMON_NAME): $(GO4MAP) $(FASTMON_ONLYMAP)
	@rm -f $(FASTMON_MAP)
	@cat $(GO4MAP) $(FASTMON_ONLYMAP) > $(FASTMON_MAP)
else
map-$(FASTMON_NAME):

endif
