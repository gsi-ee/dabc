ifndef noepics

include $(DABCSYS)/config/Makefile.config

ifdef DABC_EPICS

ifdef DABCMAINMAKE
EZCADIR = plugins/ezca
else
EZCADIR = .
endif


EZCA_DIRI         = $(EZCADIR)/ezca
EZCA_DIRS         = $(EZCADIR)/src

EZCA_LIBNAME     = $(LIB_PREFIX)DabcEzca
EZCA_LIB         = $(TGTDLLPATH)/$(EZCA_LIBNAME).$(DllSuf)

## must be similar for every module

EZCA_H           = $(wildcard $(EZCA_DIRI)/*.$(HedSuf))
EZCA_S           = $(wildcard $(EZCA_DIRS)/*.$(SrcSuf))
EZCA_O           = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(EZCA_S))
EZCA_D           = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(EZCA_S))

# used in the main Makefile

ALLDEPENDENC    += $(EZCA_D)

libs:: $(EZCA_LIB)

clean::
	@rm -f $(EZCADIR)/go4/*.o $(EZCADIR)/go4/*.d $(EZCADIR)/go4/*.root $(EZCADIR)/go4/*.so


##### local rules #####

$(EZCA_O) $(EZCA_D) : INCLUDES += $(DABC_EPICS_INCDIR) $(EZCADIR) 

$(EZCA_LIB):  $(EZCA_O) $(DABCMBS_LIB)
	@$(MakeLib) $(EZCA_LIBNAME) "$(EZCA_O)" $(TGTDLLPATH) "$(call lddiropt,$(DABC_EPICS_LIBDIR)) -lezca -lDabcMbs"

########### extra rules #############

endif

include $(DABCSYS)/config/Makefile.rules

endif

