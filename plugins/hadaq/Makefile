## normally should be like this for every plugin
## one can extract complete plugin subdirectory and compile it independent from rest

include $(DABCSYS)/config/Makefile.config

DOOPTIMIZATION = false

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


DABCHADAQ_O       = $(HADAQ_O) 
DABCHADAQ_D       = $(HADAQ_D) 


# used in the main Makefile

ALLHDRS           += $(patsubst $(HADAQDIR)/%.h, $(DABCINCPATH)/%.h, $(HADAQ_H))
ALLDEPENDENC      += $(DABCHADAQ_D)

libs:: $(DABCHADAQ_LIB)

##### local rules #####

$(DABCINCPATH)/%.h: $(HADAQDIR)/%.h
	@echo "Header: $@" 
	@cp -f $< $@

$(DABCHADAQ_LIB):   $(DABCHADAQ_O)
	@$(MakeLib) $(DABCHADAQ_LIBNAME) "$(DABCHADAQ_O)" $(TGTDLLPATH) "-lrt"


clean::
	rm -f $(HADAQFAPI_PACKAGE)

########### extra rules #############
$(HADAQ_O) $(HADAQ_D) : DEFINITIONS += $(HADAQAPI_DEFINITIONS) 
$(HADAQAPI_CO) $(HADAQAPI_CD): C_DEFINITIONS += $(HADAQAPI_DEFINITIONS)


include $(DABCSYS)/config/Makefile.rules
