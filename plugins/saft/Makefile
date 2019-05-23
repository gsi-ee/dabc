ifdef withsaft

include $(DABCSYS)/config/Makefile.config

ifdef DABC_SAFT

ifdef DABCMAINMAKE
SAFTDIR = plugins/saft
else
SAFTDIR = .
endif


SAFT_LINKLIBS     = -lsaftlib 

SAFT_DIRI         = $(SAFTDIR)/saftdabc
SAFT_DIRS         = $(SAFTDIR)/src

SAFT_LIBNAME     = $(LIB_PREFIX)DabcSaft
SAFT_LIB         = $(TGTDLLPATH)/$(SAFT_LIBNAME).$(DllSuf)

## must be similar for every module

SAFT_H           = $(wildcard $(SAFT_DIRI)/*.$(HedSuf))
SAFT_S           = $(wildcard $(SAFT_DIRS)/*.$(SrcSuf))
SAFT_O           = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(SAFT_S))
SAFT_D           = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(SAFT_S))

# used in the main Makefile

ALLHDRS           += $(patsubst $(SAFTDIR)/%.h, $(DABCINCPATH)/%.h, $(SAFT_H))
ALLDEPENDENC      += $(SAFT_D)

libs:: $(SAFT_LIB)

clean::
	@$(RM) $(SAFTDIR)/go4/*.o $(SAFTDIR)/go4/*.d $(SAFTDIR)/go4/*.root $(SAFTDIR)/go4/*.so


##### local rules #####

$(DABCINCPATH)/%.h: $(SAFTDIR)/%.h
	@echo "Header: $@" 
	@cp -f $< $@


$(SAFT_O) $(SAFT_D) : INCLUDES += $(DABC_SAFT_INCDIR)
	CXXFLAGS+=$(DABC_GIOMM_CFLAGS)

$(SAFT_LIB):  $(SAFT_O) $(DABCMBS_LIB)
	@$(MakeLib) $(SAFT_LIBNAME) "$(SAFT_O)" $(TGTDLLPATH) "$(call lddiropt,$(DABC_SAFT_LIBDIR)) $(SAFT_LINKLIBS) $(DABC_GIOMM_LIBS) -lDabcMbs"

########### extra rules #############

endif

include $(DABCSYS)/config/Makefile.rules

endif

