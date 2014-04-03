## normally should be like this for every plugin
## one can extract complete plugin subdirectory and compile it independent from rest

ifndef nofesa

include $(DABCSYS)/config/Makefile.config

ifdef DABCMAINMAKE
FESADIR = plugins/fesa
else
FESADIR = .
INCLUDES += $(FESADIR)
endif

FESADIRI         = $(FESADIR)/fesa
FESADIRS         = $(FESADIR)/src

DABCFESA_LIBNAME  = $(LIB_PREFIX)DabcFesa
DABCFESA_LIB      = $(TGTDLLPATH)/$(DABCFESA_LIBNAME).$(DllSuf)

DABCFESA_H       = $(wildcard $(FESADIRI)/*.$(HedSuf))
DABCFESA_S       = $(wildcard $(FESADIRS)/*.$(SrcSuf))
DABCFESA_O       = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(DABCFESA_S))
DABCFESA_D       = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(DABCFESA_S))


FESA_DEFINITIONS = 
FESA_INCLUDES =
FESA_LIBS = -lDabcMbs
FESA_STATIC =
FESA_DEPEND = $(DABCMBS_LIB)   

ifdef DABC_ISROOT
FESA_DEFINITIONS += WITH_ROOT 
FESA_INCLUDES += $(DABC_ROOTINCDIR)
FESA_LIBS += -lDabcRoot 
FESA_DEPEND += $(DABCROOT_LIB)
endif

# this is location of fesa 

ifdef DABC_FESA
FESA_DEFINITIONS += WITH_FESA
FESA_INCLUDES += $(DABC_FESA_INCDIR)
FESA_LIBS += $(DABC_FESA_LIBS)
FESA_STATIC += $(DABC_FESA_SLIBS)
endif


# used in the main Makefile

#ALLHDRS           += $(patsubst $(FESADIR)/%.h, $(DABCINCPATH)/%.h, $(FESA_H))
ALLDEPENDENC      += $(DABCFESA_D)

libs:: $(DABCFESA_LIB)

##### local rules #####

#$(DABCINCPATH)/%.h: $(FESADIR)/%.h
#	@echo "Header: $@" 
#	@cp -f $< $@

$(DABCFESA_LIB):   $(DABCFESA_O) $(FESA_DEPEND)  
	@$(MakeLib) $(DABCFESA_LIBNAME) "$(DABCFESA_O) $(FESA_STATIC)" $(TGTDLLPATH) "$(FESA_LIBS)" 

########### extra rules #############
$(DABCFESA_O) $(DABCFESA_D) : DEFINITIONS += $(FESA_DEFINITIONS) 
$(DABCFESA_O) $(DABCFESA_D) : INCLUDES += $(FESA_INCLUDES) $(FESADIR)

include $(DABCSYS)/config/Makefile.rules

endif