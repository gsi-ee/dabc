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
ROCDIRGO4       = $(ROCDIR)/go4monitor

ROC_LIBNAME     = $(LIB_PREFIX)DabcKnut
ROC_LIB         = $(TGTDLLPATH)/$(ROC_LIBNAME).$(DllSuf)

## must be similar for every module

ROC_H           = $(wildcard $(ROCDIRI)/*.$(HedSuf))
ROC_S           = $(wildcard $(ROCDIRS)/*.$(SrcSuf))
ROC_O           = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(ROC_S))
ROC_D           = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(ROC_S))

# used in the main Makefile

ALLHDRS         += $(patsubst $(ROCDIR)/%.h, $(DABCINCPATH)/%.h, $(ROC_H))
ALLDEPENDENC    += $(ROC_D)

libs:: $(ROC_LIB)

ifdef GO4SYS
exes::
	@echo "Build go4 monitor analsysis for roc application"
	@cd $(ROCDIRGO4); make all
endif

##### local rules #####

$(DABCINCPATH)/%.h: $(ROCDIR)/%.h
	@echo "Header: $@" 
	@cp -f $< $@

$(ROC_LIB):  $(ROC_O) $(KNUT_SLIB)
	@$(MakeLib) $(ROC_LIBNAME) "$(ROC_O) $(KNUT_SLIB)" $(TGTDLLPATH) 

########### extra rules #############
$(ROC_O) $(ROC_D): INCLUDES += $(KNUT_DIR)/Control 

include $(DABCSYS)/config/Makefile.rules

endif
