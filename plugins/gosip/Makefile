## normally should be like this for every plugin
## one can extract complete plugin subdirectory and compile it independent from rest

ifndef nogosip

include $(DABCSYS)/config/Makefile.config

ifdef DABC_GOSIP

ifdef DABCMAINMAKE
GOSIPDIR = plugins/gosip
else
GOSIPDIR = .
INCLUDES += $(GOSIPDIR)
endif

GOSIPDIRI         = $(GOSIPDIR)/gosip
GOSIPDIRS         = $(GOSIPDIR)/src

DABCGOSIP_LIBNAME  = $(LIB_PREFIX)DabcGosip
DABCGOSIP_LIB      = $(TGTDLLPATH)/$(DABCGOSIP_LIBNAME).$(DllSuf)

DABCGOSIP_H       = $(wildcard $(GOSIPDIRI)/*.$(HedSuf))
DABCGOSIP_S       = $(wildcard $(GOSIPDIRS)/*.$(SrcSuf))
DABCGOSIP_O       = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(DABCGOSIP_S))
DABCGOSIP_D       = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(DABCGOSIP_S))

# used in the main Makefile

#ALLHDRS           += $(patsubst $(GOSIPDIR)/%.h, $(DABCINCPATH)/%.h, $(GOSIP_H))
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

include $(DABCSYS)/config/Makefile.rules

endif