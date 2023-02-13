## normally should be like this for every plugin
## one can extract complete plugin subdirectory and compile it independent from rest

ifndef nodofi

include $(DABCSYS)/config/Makefile.config

ifdef DABCMAINMAKE
DOFIDIR = plugins/dofi
RDOC_EXE      = $(DABCBINPATH)/rdoficom
else
DOFIDIR = .
INCLUDES += $(DOFIDIR)
RDOC_EXE      = rdoficom
endif

ifdef DABC_DOFI

DOFIDIRI         = $(DOFIDIR)/dofi
DOFIDIRS         = $(DOFIDIR)/src

DABCDOFI_LIBNAME  = $(LIB_PREFIX)DabcDofi
DABCDOFI_LIB      = $(TGTDLLPATH)/$(DABCDOFI_LIBNAME).$(DllSuf)

DABCDOFI_H       = $(wildcard $(DOFIDIRI)/*.$(HedSuf))
DABCDOFI_S       = $(wildcard $(DOFIDIRS)/*.$(SrcSuf))
DABCDOFI_O       = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(DABCDOFI_S))
DABCDOFI_D       = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(DABCDOFI_S))

# used in the main Makefile

ALLHDRS           += $(patsubst $(DOFIDIR)/%.h, $(DABCINCPATH)/%.h, $(DABCDOFI_H))
ALLDEPENDENC      += $(DABCDOFI_D)

libs:: $(DABCDOFI_LIB)

##### local rules #####

$(DABCINCPATH)/%.h: $(DOFIDIR)/%.h
	@echo "Header: $@" 
	@cp -f $< $@

$(DABCDOFI_LIB):   $(DABCDOFI_O) $(DOFI_DEPEND)  
	@$(MakeLib) $(DABCDOFI_LIBNAME) "$(DABCDOFI_O)" $(TGTDLLPATH) "-lDabcBase" 
########### extra rules #############
#$(DABCDOFI_O) $(DABCDOFI_D) : INCLUDES += $(DABC_DOFI_INCDIR)

endif

RDOC_EXES     = $(DOFIDIR)/rdoficom.$(SrcSuf)
RDOC_EXEO     = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(RDOC_EXES))
RDOC_EXED     = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(RDOC_EXES))

ALLDEPENDENC      += $(RDOC_EXED)

$(RDOC_EXE) : $(RDOC_EXEO) $(DABCBASE_LIB)
	$(LD) $(LDFLAGSPRE) -O $(RDOC_EXEO) $(LIBS_CORESET) -o $(RDOC_EXE)


exes:: $(RDOC_EXE)

clean::
	@$(RM) $(RDOC_EXE)


include $(DABCSYS)/config/Makefile.rules

endif
