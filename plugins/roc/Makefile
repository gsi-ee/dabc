ifndef KNUT_DIR
KNUT_DIR = $(DABCSYS)/../sw-local
endif

ifndef KNUT_SLIB
KNUT_SLIB = $(KNUT_DIR)/libKnut.a
endif

ifneq ($(wildcard $(KNUT_DIR)/Control),)
ifneq ($(wildcard $(KNUT_SLIB)),)
USEKNUT    = true
endif
endif

ifdef USEKNUT 

include $(DABCSYS)/config/Makefile.config

## normally should be like this for every module, but can be specific

ifdef DABCMAINMAKE
ROCDIR = plugins/roc
else
ROCDIR = .
INCLUDES += $(ROCDIR)
endif

ROCDIRI         = $(ROCDIR)/roc
ROCDIRS         = $(ROCDIR)/src
ROCPLUGINDIR    = $(ROCDIR)/dataserver

ROC_LIBNAME     = $(LIB_PREFIX)DabcKnut
ROC_LIB         = $(TGTDLLPATH)/$(ROC_LIBNAME).$(DllSuf)

## must be similar for every module

ROC_H           = $(wildcard $(ROCDIRI)/*.$(HedSuf))
ROC_S           = $(wildcard $(ROCDIRS)/*.$(SrcSuf))
ROC_O           = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(ROC_S))
ROC_D           = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(ROC_S))

ROC_EXES        = $(ROCPLUGINDIR)/rocdataserver.$(SrcSuf)
ROC_EXEO        = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(ROC_EXES))
ROC_EXED        = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(ROC_EXES))
ROC_EXE         = $(TGTBINPATH)/rocdataserver

# used in the main Makefile

ALLHDRS         += $(patsubst $(ROCDIR)/%.h, $(DABCINCPATH)/%.h, $(ROC_H))
ALLDEPENDENC    += $(ROC_D) $(ROC_EXED)
CREATE_DIRS     += $(TGTDLLPATH) $(TGTBINPATH)
LIBS_PLUGINS    += $(ROC_LIB)

libs:: $(ROC_LIB)

exes:: $(ROC_EXE)

##### local rules #####

$(DABCINCPATH)/%.h: $(ROCDIR)/%.h
	@echo "Header: $@" 
	@cp -f $< $@

$(ROC_LIB):  $(ROC_O) $(KNUT_SLIB)
	@$(MakeLib) $(ROC_LIBNAME) "$(ROC_O) $(KNUT_SLIB)" $(TGTDLLPATH) 

$(ROC_EXE):  $(ROC_EXEO) $(ROC_LIB)
	$(LD) $(LDFLAGS) $(ROC_EXEO) $(LIBS_CORESET) -lDabcMbs -lDabcKnut $(OutPutOpt) $(ROC_EXE)

########### extra rules #############
$(ROC_O) $(ROC_D) $(ROC_EXE) $(ROC_EXEO) : INCLUDES += $(KNUT_DIR)/Control 

include $(DABCSYS)/config/Makefile.rules

endif
