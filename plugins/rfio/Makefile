## normally should be like this for every plugin
## one can extract complete plugin subdirectory and compile it independent from rest

include $(DABCSYS)/config/Makefile.config

#DOOPTIMIZATION = false

ifdef DABCMAINMAKE
RFIODIR = plugins/rfio
else
RFIODIR = .
INCLUDES += $(RFIODIR)
endif

RFIODIRI         = $(RFIODIR)/rfio
RFIODIRS         = $(RFIODIR)/src
RFIODIRAPI       = $(RFIODIR)/rawapi

DABCRFIO_LIBNAME  = $(LIB_PREFIX)DabcRfio
DABCRFIO_LIB      = $(TGTDLLPATH)/$(DABCRFIO_LIBNAME).$(DllSuf)


RFIO_H           = $(wildcard $(RFIODIRI)/*.$(HedSuf))
RFIO_S           = $(wildcard $(RFIODIRS)/*.$(SrcSuf))
RFIO_O           = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(RFIO_S))
RFIO_D           = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(RFIO_S))


RFIO_CS          = $(wildcard $(RFIODIRAPI)/*.$(CSuf))
RFIO_CO          = $(patsubst %.$(CSuf), $(BLD_DIR)/%.$(ObjSuf), $(RFIO_CS))
RFIO_CD          = $(patsubst %.$(CSuf), $(BLD_DIR)/%.$(DepSuf), $(RFIO_CS))


DABCRFIO_O       = $(RFIO_O) $(RFIO_CO)
DABCRFIO_D       = $(RFIO_D) $(RFIO_CD)


# used in the main Makefile

ALLHDRS           += $(patsubst $(RFIODIR)/%.h, $(DABCINCPATH)/%.h, $(RFIO_H))
ALLDEPENDENC      += $(DABCRFIO_D)

libs:: $(DABCRFIO_LIB)

##### local rules #####

$(DABCINCPATH)/%.h: $(RFIODIR)/%.h
	@echo "Header: $@" 
	@cp -f $< $@

$(DABCRFIO_LIB):   $(DABCRFIO_O)
	@$(MakeLib) $(DABCRFIO_LIBNAME) "$(DABCRFIO_O)" $(TGTDLLPATH)

clean::
#	rm -f $(RFIOFAPI_PACKAGE)

########### extra rules #############
$(RFIO_O) $(RFIO_D) : INCLUDES += $(RFIODIRAPI)  

$(RFIO_CO) $(RFIO_CD): C_DEFINITIONS += Linux

ifdef DABC_64BIT 
$(RFIO_CO) $(RFIO_CD): C_DEFINITIONS += SYSTEM64 
endif

include $(DABCSYS)/config/Makefile.rules
