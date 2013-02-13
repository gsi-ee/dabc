## normally should be like this for every plugin
## one can extract complete plugin subdirectory and compile it independent from rest

include $(DABCSYS)/config/Makefile.config

#DOOPTIMIZATION = false

ifdef DABCMAINMAKE
HADAQDIR = plugins/hadaq
else
HADAQDIR = .
INCLUDES += $(HADAQDIR)
endif

HADAQDIRI         = $(HADAQDIR)/hadaq
HADAQDIRS         = $(HADAQDIR)/src

DABCHADAQ_LIBNAME  = $(LIB_PREFIX)DabcHadaq
DABCHADAQ_LIB      = $(TGTDLLPATH)/$(DABCHADAQ_LIBNAME).$(DllSuf)


HADAQ_EXCLUDE      = 

HADAQ_H           = $(filter-out $(HADAQ_EXCLUDE), $(wildcard $(HADAQDIRI)/*.$(HedSuf)))
HADAQ_S           = $(filter-out $(HADAQ_EXCLUDE), $(wildcard $(HADAQDIRS)/*.$(SrcSuf)))
HADAQ_O           = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(HADAQ_S))
HADAQ_D           = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(HADAQ_S))


HADAQ_CS       = $(wildcard $(HADAQDIRS)/*.$(CSuf))
HADAQ_CO       = $(patsubst %.$(CSuf), $(BLD_DIR)/%.$(ObjSuf), $(HADAQ_CS))
HADAQ_CD       = $(patsubst %.$(CSuf), $(BLD_DIR)/%.$(DepSuf), $(HADAQ_CS))




DABCHADAQ_O       = $(HADAQ_O) $(HADAQ_CO)
DABCHADAQ_D       = $(HADAQ_D) $(HADAQ_CD)


# used in the main Makefile

ALLHDRS           += $(patsubst $(HADAQDIR)/%.h, $(DABCINCPATH)/%.h, $(HADAQ_H))
ALLDEPENDENC      += $(DABCHADAQ_D)

libs:: $(DABCHADAQ_LIB)

##### local rules #####

$(DABCINCPATH)/%.h: $(HADAQDIR)/%.h
	@echo "Header: $@" 
	@cp -f $< $@

$(DABCHADAQ_LIB): LDFLAGS += -lrt

$(DABCHADAQ_LIB):   $(DABCHADAQ_O)
	@$(MakeLib) $(DABCHADAQ_LIBNAME) "$(DABCHADAQ_O)" $(TGTDLLPATH)


clean::
#	rm -f $(HADAQFAPI_PACKAGE)

########### extra rules #############
#$(HADAQ_O) $(HADAQ_D) : DEFINITIONS += $(HADAQAPI_DEFINITIONS) 
$(HADAQ_CO) $(HADAQ_CD): C_INCLUDES += -I $(HADAQDIRI) 

include $(DABCSYS)/config/Makefile.rules
