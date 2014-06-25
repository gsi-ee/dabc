## normally should be like this for every plugin
## one can extract complete plugin subdirectory and compile it independent from rest

include $(DABCSYS)/config/Makefile.config

#DOOPTIMIZATION = false

ifdef DABCMAINMAKE
HADAQDIR = plugins/hadaq
HLDPRINT_EXE      = $(DABCBINPATH)/hldprint
else
HADAQDIR = .
INCLUDES += $(HADAQDIR)
HLDPRINT_EXE      = hldprint
endif


HADAQDIRI         = $(HADAQDIR)/hadaq
HADAQDIRS         = $(HADAQDIR)/src

# now defined in the Makefile.config
#DABCHADAQ_LIBNAME = $(LIB_PREFIX)DabcHadaq
#DABCHADAQ_LIB     = $(TGTDLLPATH)/$(DABCHADAQ_LIBNAME).$(DllSuf)

HADAQ_H           = $(wildcard $(HADAQDIRI)/*.$(HedSuf))
HADAQ_S           = $(wildcard $(HADAQDIRS)/*.$(SrcSuf))
HADAQ_O           = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(HADAQ_S))
HADAQ_D           = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(HADAQ_S))

HADAQ_CS          = $(wildcard $(HADAQDIRS)/*.$(CSuf))
HADAQ_CO          = $(patsubst %.$(CSuf), $(BLD_DIR)/%.$(ObjSuf), $(HADAQ_CS))
HADAQ_CD          = $(patsubst %.$(CSuf), $(BLD_DIR)/%.$(DepSuf), $(HADAQ_CS))

HLDPRINT_EXES     = $(HADAQDIR)/hldprint.$(SrcSuf)
HLDPRINT_EXEO     = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(HLDPRINT_EXES))
HLDPRINT_EXED     = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(HLDPRINT_EXES))

DABCHADAQ_O       = $(HADAQ_O) $(HADAQ_CO)
DABCHADAQ_D       = $(HADAQ_D) $(HADAQ_CD)

# used in the main Makefile

ALLHDRS           += $(patsubst $(HADAQDIR)/%.h, $(DABCINCPATH)/%.h, $(HADAQ_H))
ALLDEPENDENC      += $(DABCHADAQ_D) $(HLDPRINT_EXED)

libs:: $(DABCHADAQ_LIB)

exes:: $(HLDPRINT_EXE)

clean::
	rm -f $(HLDPRINT_EXE)


##### local rules #####

$(DABCINCPATH)/%.h: $(HADAQDIR)/%.h
	@echo "Header: $@" 
	@cp -f $< $@

$(DABCHADAQ_LIB):   $(DABCHADAQ_O)
	@$(MakeLib) $(DABCHADAQ_LIBNAME) "$(DABCHADAQ_O)" $(TGTDLLPATH) "-lrt"

$(HLDPRINT_EXE) : $(HLDPRINT_EXEO) $(DABCBASE_LIB) $(DABCMBS_LIB) $(DABCHADAQ_LIB)
	$(LD) -O $(HLDPRINT_EXEO) -Wl,--no-as-needed $(LIBS_CORESET) -lDabcMbs -lDabcHadaq -o $(HLDPRINT_EXE)

########### extra rules #############
$(HADAQ_CO) $(HADAQ_CD): C_INCLUDES += -I $(HADAQDIRI) 

include $(DABCSYS)/config/Makefile.rules
