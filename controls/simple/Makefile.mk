#this is not a plugin, therefore cannot be extracted from main makefile

SCTRLDIR         = controls/simple
SCTRLDIRI        = $(SCTRLDIR)/dabc
SCTRLDIRS        = $(SCTRLDIR)/src
SCTRLTEST1DIR    = $(SCTRLDIR)/test1
SCTRLTEST2DIR    = $(SCTRLDIR)/test2

## must be similar for every module

SCTRL_H           = $(wildcard $(SCTRLDIRI)/*.$(HedSuf))
SCTRL_S           = $(wildcard $(SCTRLDIRS)/*.$(SrcSuf))
SCTRL_O           = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(SCTRL_S))
SCTRL_D           = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(SCTRL_S))

# used in the main Makefile

ALLHDRS            += $(patsubst $(SCTRLDIR)/%.h, $(DABCINCPATH)/%.h, $(SCTRL_H))
ALLDEPENDENC       += $(SCTRL_D) 
APPLICATIONS_DIRS  += $(SCTRLTEST1DIR) $(SCTRLTEST2DIR) 

libs:: $(DABCSCTRL_LIB) $(DABCSCTRL_SLIB)

##### local rules #####

$(DABCINCPATH)/%.h: $(SCTRLDIR)/%.h
	@echo "Header: $@" 
	@cp -f $< $@

$(DABCSCTRL_LIB):   $(SCTRL_O)
	@$(MakeLib) $(DABCSCTRL_LIBNAME) "$(SCTRL_O)" $(DABCDLLPATH)

$(DABCSCTRL_SLIB):   $(SCTRL_O)
	$(AR) $(ARFLAGS) $(DABCSCTRL_SLIB) $(SCTRL_O)
	