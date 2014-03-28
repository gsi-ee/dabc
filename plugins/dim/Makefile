include $(DABCSYS)/config/Makefile.config

ifdef DABC_DIM

ifdef DABCMAINMAKE
DIMCTLDIR = plugins/dim
else
DIMCTLDIR = .
endif

################## building of DABC plugin ###################

DIMCTLDIRI         = $(DIMCTLDIR)/dimc
DIMCTLDIRS         = $(DIMCTLDIR)/src

DIMCTL_LIBNAME     = $(LIB_PREFIX)DabcDim
DIMCTL_LIB         = $(TGTDLLPATH)/$(DIMCTL_LIBNAME).$(DllSuf)

DIMCTL_EXCLUDE      = 

DIMCTL_H           = $(filter-out $(DIMCTL_EXCLUDE), $(wildcard $(DIMCTLDIRI)/*.$(HedSuf)))
DIMCTL_S           = $(filter-out $(DIMCTL_EXCLUDE), $(wildcard $(DIMCTLDIRS)/*.$(SrcSuf)))
DIMCTL_O           = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(DIMCTL_S))
DIMCTL_D           = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(DIMCTL_S))

ALLHDRS           += $(patsubst $(DIMCTLDIR)/%.h, $(DABCINCPATH)/%.h, $(DIMCTL_H))
ALLDEPENDENC      += $(DIMCTL_D)

libs:: $(DIMCTL_LIB)

$(DABCINCPATH)/%.h: $(DIMCTLDIR)/%.h
	@echo "Header: $@" 
	@cp -f $< $@

$(DIMCTL_LIB): LDDIRS += $(DABC_DIM_LIBDIR)
$(DIMCTL_LIB): LDFLAGS += -ldim

$(DIMCTL_LIB):  $(DIMCTL_O) 
	@$(MakeLib) $(DIMCTL_LIBNAME) "$(DIMCTL_O)" $(TGTDLLPATH)

$(DIMCTL_O) $(DIMCTL_D): INCLUDES += $(DABC_DIM_INCDIR)

endif

include $(DABCSYS)/config/Makefile.rules
