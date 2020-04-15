## normally should be like this for every plugin
## one can extract complete plugin subdirectory and compile it independent from rest

ifndef norfio

include $(DABCSYS)/config/Makefile.config

ifdef DABCMAINMAKE
RFIODIR = plugins/rfio
else
RFIODIR = .
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


DABCRFIO_O       = $(RFIO_O) 
DABCRFIO_D       = $(RFIO_D)

ifdef ADSMDIR
DABCRFIO_O       += $(wildcard $(ADSMDIR)/*.a)
$(RFIO_O) $(RFIO_D) : INCLUDES += $(ADSMDIR)
$(RFIO_O) $(RFIO_D) : DEFINITIONS += ORIGIN_ADSM 
else
DABCRFIO_O       += $(RFIO_CO)
DABCRFIO_D       += $(RFIO_CD)

$(RFIO_O) $(RFIO_D) : INCLUDES += $(RFIODIRAPI)  
endif

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
#	$(RM) $(RFIOFAPI_PACKAGE)

########### extra rules #############

$(RFIO_CO) $(RFIO_CD): C_DEFINITIONS += Linux

ifneq ($(CC),clang)
GCCMAJOR = $(shell $(CC) -dumpversion | cut -f1 -d.)
ifeq ($(shell test $(GCCMAJOR) -gt 7; echo $$?),0)
$(RFIO_CO): CFLAGS += -Wno-stringop-overflow
endif
endif

$(RFIO_O) : CXXFLAGS += -Wno-unused-variable 

ifdef DABC_64BIT 
$(RFIO_CO) $(RFIO_CD): C_DEFINITIONS += SYSTEM64 
endif

include $(DABCSYS)/config/Makefile.rules

endif
