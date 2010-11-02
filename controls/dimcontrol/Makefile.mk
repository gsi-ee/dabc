#this is not a plugin, therefore cannot be extracted from main makefile

ifdef DIMDIR

DIM_INCLUDES       = $(DIMDIR)/dim

DIMCTRLDIR         = controls/dimcontrol
DIMCTRLDIRI        = $(DIMCTRLDIR)/dimc
DIMCTRLDIRS        = $(DIMCTRLDIR)/src
#DIMCTRLTEST1DIR    = $(DIMCTRLDIR)/test1
#DIMCTRLTEST2DIR    = $(DIMCTRLDIR)/test2

## must be similar for every module

DIMCTRL_H           = $(wildcard $(DIMCTRLDIRI)/*.$(HedSuf))
DIMCTRL_S           = $(wildcard $(DIMCTRLDIRS)/*.$(SrcSuf))
DIMCTRL_O           = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(DIMCTRL_S))
DIMCTRL_D           = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(DIMCTRL_S))


DABCDIMCTRL_LIBNAME  = $(LIB_PREFIX)DabcDimCtrl
DABCDIMCTRL_LIB      = $(DABCDLLPATH)/$(DABCDIMCTRL_LIBNAME).$(DllSuf)

# used in the main Makefile

ALLHDRS            += $(patsubst $(DIMCTRLDIR)/%.h, $(DABCINCPATH)/%.h, $(DIMCTRL_H))
ALLDEPENDENC       += $(DIMCTRL_D) 
#APPLICATIONS_DIRS  += $(DIMCTRLTEST1DIR) $(DIMCTRLTEST2DIR) 

#libs:: echo "make libs of dimc still not active."

libs:: $(DABCDIMCTRL_LIB) 
#$(DABCDIMCTRL_SLIB)

##### local rules #####

$(DABCINCPATH)/%.h: $(DIMCTRLDIR)/%.h
	@echo "Header: $@" 
	@cp -f $< $@

$(DABCDIMCTRL_LIB):   $(DIMCTRL_O) 
	@$(MakeLib) $(DABCDIMCTRL_LIBNAME) "$(DIMCTRL_O)" $(DABCDLLPATH) "-L$(DABCDLLPATH) -ldim"

$(DABCDIMCTRL_SLIB):   $(DIMCTRL_O)
	$(AR) $(ARFLAGS) $(DABCDIMCTRL_SLIB) $(DIMCTRL_O)
	
$(DIMCTRL_O) $(DIMCTRL_D): INCLUDES += $(DIM_INCLUDES) 	

$(DIMCTRL_O) $(DIMCTRL_D): $(DIM_LIB)

else

libs:: 
	@echo "make libs of dimc not active while DIMDIR is not specified."

endif	