ifndef ROCSYS
ifneq ($(wildcard ../ROC/config/Makefile.arch),)
ROCSYS=../ROC
$(info using ROClib from ../ROC directory)
endif
endif

ifdef ROCSYS

-include $(ROCSYS)/config/Makefile.arch

include $(DABCSYS)/config/Makefile.config

## normally should be like this for every module, but can be specific

ROC_EXTRALIBS = 

ROCPLUGDIR      = plugins/roc
ROCDIRI         = $(ROCPLUGDIR)/roc
ROCDIRS         = $(ROCPLUGDIR)/src

ifdef ROCTGTDIR
ROCPLUGIN_TGTDIR     = $(ROCTGTDIR)/lib
else
ROCPLUGIN_TGTDIR     = $(TGTDLLPATH)
endif

ROC_LIBNAME     = $(LIB_PREFIX)DabcRoc
ROC_LIB         = $(ROCPLUGIN_TGTDIR)/$(ROC_LIBNAME).$(DllSuf)

## must be similar for every module

ROC_H           = $(wildcard $(ROCDIRI)/*.$(HedSuf))
ROC_S           = $(wildcard $(ROCDIRS)/*.$(SrcSuf))
ROC_O           = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(ROC_S))
ROC_D           = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(ROC_S))

# used in the main Makefile

ALLHDRS         += $(patsubst $(ROCPLUGDIR)/%.h, $(DABCINCPATH)/%.h, $(ROC_H))
ALLDEPENDENC    += $(ROC_D)

libs:: $(ROC_LIB) 

##### local rules #####

$(DABCINCPATH)/%.h: $(ROCPLUGDIR)/%.h
	@echo "Header: $@" 
	@cp -f $< $@

$(ROC_O) $(ROC_D) : INCLUDES += $(ROCSYS)/include .

$(ROC_LIB): LDFLAGS += -L$(ROCSYS)/lib -lRoc -L$(DABC_LIBDIR) -lDabcBase -lDabcMbs 

$(ROC_LIB):  $(ROC_O)
	@$(MakeLib) $(ROC_LIBNAME) "$(ROC_O)" $(ROCPLUGIN_TGTDIR)

########### extra rules #############

include $(DABCSYS)/config/Makefile.rules

endif
