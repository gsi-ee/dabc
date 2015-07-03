ifndef noroot

include $(DABCSYS)/config/Makefile.config

ifeq ($(DABC_ROOT_RHHTP), true)

ifdef DABCMAINMAKE
DABCROOT_DIR  = plugins/root
DABCROOT_EXE  = $(DABCBINPATH)/dabc_root
else
DABCROOT_DIR = .
DABCROOT_EXE  = dabc_root
endif

DABCROOT_DIRI      = $(DABCROOT_DIR)/root
DABCROOT_DIRS      = $(DABCROOT_DIR)/src

# move it to Makefile.config, while ROOT plugin used in some others plugins
#DABCROOT_LIBNAME  = $(LIB_PREFIX)DabcRoot
#DABCROOT_LIB      = $(TGTDLLPATH)/$(DABCROOT_LIBNAME).$(DllSuf)
#DABCROOT_MAP      = $(TGTDLLPATH)/$(DABCROOT_LIBNAME).rootmap

DABCROOT_TOPINCL = $(DABCROOT_DIRI)/TDabcEngine.h

DABCROOT_DICTINCL = $(DABCROOT_TOPINCL)
                    
DABCROOT_LIBDEP   = $(TGTDLLPATH)/libDabcHttp.$(DllSuf) \
                    $(TGTDLLPATH)/libDabcBase.$(DllSuf)

DABCROOT_EXTRALIBS = -L$(TGTDLLPATH) -lDabcBase -lDabcHttp

## must be similar for every module

DABCROOT_H        = $(wildcard $(DABCROOT_DIRI)/*.$(HedSuf))
DABCROOT_S        = $(wildcard $(DABCROOT_DIRS)/*.$(SrcSuf))
DABCROOT_O        = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(DABCROOT_S))
DABCROOT_D        = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(DABCROOT_S))

DABCROOT_DICT      = $(BLD_DIR)/$(DABCROOT_DIR)/G__DABCROOT
DABCROOT_DH        = $(DABCROOT_DICT).$(HedSuf)
DABCROOT_DS        = $(DABCROOT_DICT).$(SrcSuf)
DABCROOT_DO        = $(DABCROOT_DICT).$(ObjSuf) 
DABCROOT_DD        = $(DABCROOT_DICT).$(DepSuf)

DABCROOT_EXES     = $(DABCROOT_DIR)/utils/dabc_root.$(SrcSuf)
DABCROOT_EXEO     = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(DABCROOT_EXES))
DABCROOT_EXED     = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(DABCROOT_EXES))


DABCROOT_ALL_O     = $(DABCROOT_O)

# used in the main Makefile

ALLHDRS          += $(patsubst $(DABCROOT_DIR)/%.h, $(DABCINCPATH)/%.h, $(DABCROOT_H))
ALLHDRS          += $(patsubst $(DABCROOT_DIR)/%.h, $(DABCINCPATH)/%.h, $(DABCROOT_TOPINCL)) 
ALLDEPENDENC     += $(DABCROOT_D) $(DABCROOT_DD) $(DABCROOT_EXED)  

# when using ROOT libRHTTP 

DABCROOT_LINKDEF  = $(DABCROOT_DIR)/LinkDef.h

DABCROOT_LIBDEP += $(DABC_ROOTLIBDIR)/libRHTTP.$(DllSuf)

HTTPLIBEXTRA += -lRHTTP -lXMLIO


libs:: $(DABCROOT_LIB) $(DABCROOT_MAP)

exes:: $(DABCROOT_EXE)

clean::
	rm -f $(DABCROOT_EXE)


##### local rules #####

ifeq ($(DABC_ROOTMAJORVER),6)
HTTPLIBEXTRA += $(DABCROOT_EXTRALIBS)
endif

$(DABCINCPATH)/%.h: $(DABCROOT_DIR)/%.h
	@echo "Header: $@" 
	@cp -f $< $@

ifeq ($(DABC_ROOT_RHHTP),false)

$(DABCINCPATH)/%.h: $(DABCROOT_DIRSNIFF)/%.h
	@echo "Header: $@" 
	@cp -f $< $@

$(DABCINCPATH)/%.h: $(DABCROOT_DIRVER)/%.h
	@echo "Header: $@" 
	@cp -f $< $@

endif


$(DABCROOT_LIB): $(DABCROOT_ALL_O) $(DABCROOT_DO)
	$(MakeLib) $(DABCROOT_LIBNAME) "$(DABCROOT_ALL_O) $(DABCROOT_DO)" $(TGTDLLPATH) "$(DABC_ROOTLIBS) $(HTTPLIBEXTRA)"

$(DABCROOT_DS): $(DABCROOT_DICTINCL) $(DABCROOT_LINKDEF)
	$(DABC_ROOTCINT) $(DABCROOT_LIB) "$(DABCROOT_DICTINCL) $(DABCROOT_LINKDEF)"

$(DABCROOT_MAP) : $(DABCROOT_LIB) $(DABCROOT_LINKDEF) 
	$(DABC_ROOTMAP) $(DABCROOT_LIB) "$(DABCROOT_LIBDEP)" $(DABCROOT_DS) $(DABCROOT_LINKDEF) "$(DABCROOT_DICTINCL)"

$(DABCROOT_EXE) : $(DABCROOT_EXEO) $(DABCBASE_LIB) $(DABCROOT_LIB)
	$(LD) -O $(DABCROOT_EXEO) -Wl,--no-as-needed $(LIBS_CORESET) -lDabcRoot $(DABC_ROOTLIBS) -o $(DABCROOT_EXE)

########### extra rules #############

$(DABCROOT_O) $(DABCROOT_DO) $(DABCROOT_D) $(DABCROOT_DD) $(DABCROOT_EXEO) $(DABCROOT_EXED) : CXXFLAGS += $(DABC_ROOTCFLAGS)

$(DABCROOT_DO) : INCLUDES += .

$(DABCROOT_D) $(DABCROOT_DD) $(DABCROOT_EXED) : INCLUDES += $(DABCROOT_DIR) .

endif

include $(DABCSYS)/config/Makefile.rules

endif
