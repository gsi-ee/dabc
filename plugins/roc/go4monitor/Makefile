#general make file for any Go4 module

include $(GO4SYS)/Makefile.config

# for compile example under cygwin, 
# relative path to GO4SYS directory should be specified
ifdef GO4_WIN32
  GO4SYS = ..
endif   

# comment to switch on optimization
# DOOPTIMIZATION = false

# to add some more definitions, which can be used in #ifdef inside user code,
# just uncomment and edit following line
#DEFINITIONS += -D__USERDEF__

# if there are include files in other directories,
# just uncomment and edit following file
#INCLUDES += -I/home/usr/special/includes

# if some special libraries must be linked to the analysis
# uncomment and edit following line
#LIBS_FULLSET += -L/home/usr/special/lib -lspecial

all: build



GO4ROC_NAME        = FastRoc
MODULE_NAME        = $(GO4ROC_NAME)

## normally should be like this for every module, but can be specific

GO4ROC_DIR         = .

GO4ROC_LINKDEF     = $(GO4ROC_DIR)/LinkDef.$(HedSuf)
GO4ROC_LIBNAME     = libGo4UserAnalysis
GO4ROC_EXENAME     = MainUserAnalysis
GO4ROC_ONLYMAP     = $(GO4ROC_DIR)/.localmap
GO4ROC_MAP         = $(GO4ROC_DIR)/$(ROOTMAPNAME)

GO4ROC_NOTLIBF     = 

## must be similar for every module

GO4ROC_EXEO        = $(GO4ROC_DIR)/$(GO4ROC_EXENAME).$(ObjSuf)
GO4ROC_EXES        = $(GO4ROC_DIR)/$(GO4ROC_EXENAME).$(SrcSuf)
GO4ROC_EXE         = $(GO4ROC_DIR)/$(GO4ROC_EXENAME)$(ExeSuf)

GO4ROC_DICT        = $(GO4ROC_DIR)/$(DICT_PREFIX)$(GO4ROC_NAME)
GO4ROC_DH          = $(GO4ROC_DICT).$(HedSuf)
GO4ROC_DS          = $(GO4ROC_DICT).$(SrcSuf)
GO4ROC_DO          = $(GO4ROC_DICT).$(ObjSuf)

GO4ROC_H           = $(filter-out $(GO4ROC_EXEH) $(GO4ROC_NOTLIBF) $(GO4ROC_DH) $(GO4ROC_LINKDEF), $(wildcard $(GO4ROC_DIR)/*.$(HedSuf)))
GO4ROC_S           = $(filter-out $(GO4ROC_EXES) $(GO4ROC_NOTLIBF) $(GO4ROC_DS), $(wildcard $(GO4ROC_DIR)/*.$(SrcSuf)))
GO4ROC_O           = $(GO4ROC_S:.$(SrcSuf)=.$(ObjSuf))

GO4ROC_DEP         =  $(GO4ROC_O:.$(ObjSuf)=.$(DepSuf))
GO4ROC_DDEP        =  $(GO4ROC_DO:.$(ObjSuf)=.$(DepSuf))
GO4ROC_EDEP        =  $(GO4ROC_EXEO:.$(ObjSuf)=.$(DepSuf))

GO4ROC_SLIB        =  $(GO4ROC_DIR)/$(GO4ROC_LIBNAME).$(DllSuf)
GO4ROC_LIB         =  $(GO4ROC_DIR)/$(GO4ROC_LIBNAME).$(DllSuf).$(VERSSUF)

# used in the main Makefile

EXAMPDEPENDENCS    += $(GO4ROC_DEP) $(GO4ROC_DDEP) $(GO4ROC_EDEP)

ifdef DOPACKAGE
DISTRFILES         += $(GO4ROC_S) $(GO4ROC_H) $(GO4ROC_LINKDEF) $(GO4ROC_EXEH) $(GO4ROC_EXES)
DISTRFILES         += $(GO4ROC_DIR)/Readme.txt
DISTRFILES         += $(GO4ROC_DIR)/AnalysisStart.sh
endif

##### local rules #####

$(GO4ROC_EXE):      $(BUILDGO4LIBS) $(GO4ROC_EXEO) $(GO4ROC_LIB)
	$(LD) $(LDFLAGS) $(GO4ROC_EXEO) $(LIBS_FULLSET) $(GO4ROC_LIB) $(OutPutOpt) $(GO4ROC_EXE)
	@echo "$@  done"

$(GO4ROC_LIB):   $(GO4ROC_O) $(GO4ROC_DO)
	@$(MakeLib) $(GO4ROC_LIBNAME) "$(GO4ROC_O) $(GO4ROC_DO)" $(GO4ROC_DIR)

$(GO4ROC_DS): $(GO4ROC_H)  $(GO4ROC_LINKDEF)
	$(ROOTCINTGO4) $(GO4ROC_H) $(GO4ROC_LINKDEF)

$(GO4ROC_ONLYMAP): $(GO4ROC_LINKDEF) $(GO4ROC_LIB)
	@rm -f $(GO4ROC_ONLYMAP)
	@$(MakeMap) $(GO4ROC_ONLYMAP) $(GO4ROC_SLIB) $(GO4ROC_LINKDEF) "$(ANAL_LIB_DEP)"

all-$(GO4ROC_NAME):     $(GO4ROC_LIB) $(GO4ROC_EXE) map-$(GO4ROC_NAME)

clean-obj-$(GO4ROC_NAME):
	@rm -f $(GO4ROC_O) $(GO4ROC_DO)
	@$(CleanLib) $(GO4ROC_LIBNAME) $(GO4ROC_DIR)
	@rm -f $(GO4ROC_EXEO) $(GO4ROC_EXE)

clean-$(GO4ROC_NAME): clean-obj-$(GO4ROC_NAME)
	@rm -f $(GO4ROC_DEP) $(GO4ROC_DDEP) $(GO4ROC_DS) $(GO4ROC_DH)
	@rm -f $(GO4ROC_EDEP)
	@rm -f $(GO4ROC_ONLYMAP) $(GO4ROC_MAP)

ifdef DOMAP
map-$(GO4ROC_NAME): $(GO4MAP) $(GO4ROC_ONLYMAP)
	@rm -f $(GO4ROC_MAP)
	@cat $(GO4MAP) $(GO4ROC_ONLYMAP) > $(GO4ROC_MAP)
else
map-$(GO4ROC_NAME):

endif


build: all-$(MODULE_NAME)

clean-obj: clean-obj-$(MODULE_NAME)
	@echo "Clean objects in $(MODULE_NAME)"

clean: clean-$(MODULE_NAME)
	@echo "Clean everything in $(MODULE_NAME)"

map: map-$(MODULE_NAME)

ifdef DOPACKAGE
package:
	@tar cf $(MODULE_NAME).tar Makefile
	@tar rf $(MODULE_NAME).tar $(subst $(CURDIR),.,$(DISTRFILES))
	@gzip -f $(MODULE_NAME).tar
	@echo "$(MODULE_NAME).tar.gz done"
endif

include $(GO4SYS)/Makefile.rules
