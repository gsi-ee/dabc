## normally should be like this for every plugin
## one can extract complete plugin subdirectory and compile it independent from rest

ifndef noaqua

include $(DABCSYS)/config/Makefile.config

ifdef DABCMAINMAKE
AQUADIR = plugins/aqua
AQUATEST_EXE  = $(DABCBINPATH)/aquatest
else
AQUADIR = .
INCLUDES += $(AQUADIR)
AQUATEST_EXE  = aquatest
endif

AQUADIRI         = $(AQUADIR)/aqua
AQUADIRS         = $(AQUADIR)/src

DABCAQUA_LIBNAME  = $(LIB_PREFIX)DabcAqua
DABCAQUA_LIB      = $(TGTDLLPATH)/$(DABCAQUA_LIBNAME).$(DllSuf)

DABCAQUA_H       = $(wildcard $(AQUADIRI)/*.$(HedSuf))
DABCAQUA_S       = $(wildcard $(AQUADIRS)/*.$(SrcSuf))
DABCAQUA_O       = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(DABCAQUA_S))
DABCAQUA_D       = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(DABCAQUA_S))

AQUA_DEFINITIONS = 
AQUA_INCLUDES =
AQUA_LIBS = 
AQUA_STATIC =
AQUA_DEPEND =   

# this is location of fesa 

ifdef DABC_AQUA
AQUA_DEFINITIONS += WITH_AQUA
AQUA_INCLUDES += $(DABC_AQUA_INCDIR)
AQUA_LIBS += $(DABC_AQUA_LIBS)
AQUA_STATIC += $(DABC_AQUA_SLIBS)
endif

# used in the main Makefile

ALLHDRS           += $(patsubst $(AQUADIR)/%.h, $(DABCINCPATH)/%.h, $(AQUA_H))
ALLDEPENDENC      += $(DABCAQUA_D)

libs:: $(DABCAQUA_LIB)

exes:: $(AQUATEST_EXE)

##### local rules #####

$(DABCINCPATH)/%.h: $(AQUADIR)/%.h
	@echo "Header: $@" 
	@cp -f $< $@

$(DABCAQUA_LIB):   $(DABCAQUA_O) $(AQUA_DEPEND)  
	@$(MakeLib) $(DABCAQUA_LIBNAME) "$(DABCAQUA_O) $(AQUA_STATIC)" $(TGTDLLPATH) "$(AQUA_LIBS)" 

$(AQUATEST_EXE) : $(AQUADIR)/aquatest.c
	$(CC) $< -o $@

########### extra rules #############
$(DABCAQUA_O) $(DABCAQUA_D) : DEFINITIONS += $(AQUA_DEFINITIONS) 
$(DABCAQUA_O) $(DABCAQUA_D) : INCLUDES += $(AQUA_INCLUDES) $(AQUADIR)

include $(DABCSYS)/config/Makefile.rules

endif