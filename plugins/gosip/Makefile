## normally should be like this for every plugin
## one can extract complete plugin subdirectory and compile it independent from rest

ifndef nogosip

include $(DABCSYS)/config/Makefile.config

ifdef DABCMAINMAKE
GOSIPDIR = plugins/gosip
RGOC_EXE      = $(DABCBINPATH)/rgoc
else
GOSIPDIR = .
INCLUDES += $(GOSIPDIR)
RGOC_EXE      = rgoc
endif

ifdef DABC_GOSIP

GOSIPDIRI         = $(GOSIPDIR)/gosip
GOSIPDIRS         = $(GOSIPDIR)/src

DABCGOSIP_LIBNAME  = $(LIB_PREFIX)DabcGosip
DABCGOSIP_LIB      = $(TGTDLLPATH)/$(DABCGOSIP_LIBNAME).$(DllSuf)

DABCGOSIP_H       = $(wildcard $(GOSIPDIRI)/*.$(HedSuf))
DABCGOSIP_S       = $(wildcard $(GOSIPDIRS)/*.$(SrcSuf))
DABCGOSIP_O       = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(DABCGOSIP_S))
DABCGOSIP_D       = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(DABCGOSIP_S))

# used in the main Makefile

ALLHDRS           += $(patsubst $(GOSIPDIR)/%.h, $(DABCINCPATH)/%.h, $(DABCGOSIP_H))
ALLDEPENDENC      += $(DABCGOSIP_D)

libs:: $(DABCGOSIP_LIB)

##### local rules #####

#$(DABCINCPATH)/%.h: $(GOSIPDIR)/%.h
#	@echo "Header: $@" 
#	@cp -f $< $@

$(DABCGOSIP_LIB):   $(DABCGOSIP_O) $(GOSIP_DEPEND)  
	@$(MakeLib) $(DABCGOSIP_LIBNAME) "$(DABCGOSIP_O)" $(TGTDLLPATH) "$(call lddiropt,$(DABC_GOSIP_LIBDIR)) -lmbspex -lDabcMbs -lDabcMbs" 

########### extra rules #############
$(DABCGOSIP_O) $(DABCGOSIP_D) : INCLUDES += $(DABC_GOSIP_INCDIR)

endif

RGOC_EXES     = $(GOSIPDIR)/rgoc.$(SrcSuf)
RGOC_EXEO     = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(RGOC_EXES))
RGOC_EXED     = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(RGOC_EXES))

ALLDEPENDENC      += $(RGOC_EXED)

$(RGOC_EXE) : $(RGOC_EXEO) $(DABCBASE_LIB)
	$(LD) $(LDFLAGSPRE) -O $(RGOC_EXEO) $(LIBS_CORESET) -o $(RGOC_EXE)


exes:: $(RGOC_EXE)

clean::
	@$(RM) $(RGOC_EXE)


include $(DABCSYS)/config/Makefile.rules

endif