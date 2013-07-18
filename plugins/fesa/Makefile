## normally should be like this for every plugin
## one can extract complete plugin subdirectory and compile it independent from rest

include $(DABCSYS)/config/Makefile.config

ifdef DABCMAINMAKE
FESADIR = plugins/fesa
else
FESADIR = .
INCLUDES += $(FESADIR)
endif

# this is location of fesa 
FESA_DEFINITIONS = 
FESA_INCLUDES = 


ifdef DABC_ISROOT
FESA_DEFINITIONS += WITH_ROOT 
FESA_INCLUDES += $(DABC_ROOTINCDIR) 
endif


FESADIRI         = $(FESADIR)/fesa
FESADIRS         = $(FESADIR)/src

DABCFESA_LIBNAME  = $(LIB_PREFIX)DabcFesa
DABCFESA_LIB      = $(TGTDLLPATH)/$(DABCFESA_LIBNAME).$(DllSuf)

FESA_H           = $(wildcard $(FESADIRI)/*.$(HedSuf))
FESA_S           = $(wildcard $(FESADIRS)/*.$(SrcSuf))
FESA_O           = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(FESA_S))
FESA_D           = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(FESA_S))

DABCFESA_O       = $(FESA_O)
DABCFESA_D       = $(FESA_D)

# used in the main Makefile

#ALLHDRS           += $(patsubst $(FESADIR)/%.h, $(DABCINCPATH)/%.h, $(FESA_H))
ALLDEPENDENC      += $(DABCFESA_D)

libs:: $(DABCFESA_LIB)

##### local rules #####

#$(DABCINCPATH)/%.h: $(FESADIR)/%.h
#	@echo "Header: $@" 
#	@cp -f $< $@

#$(DABCFESA_LIB): LDFLAGS += -lrt

$(DABCFESA_LIB):   $(DABCFESA_O)
	@$(MakeLib) $(DABCFESA_LIBNAME) "$(DABCFESA_O)" $(TGTDLLPATH) 

########### extra rules #############
$(FESA_O) $(FESA_D) : DEFINITIONS += $(FESA_DEFINITIONS) 
$(FESA_O) $(FESA_D) : INCLUDES += $(FESA_INCLUDES) $(FESADIR)

include $(DABCSYS)/config/Makefile.rules
