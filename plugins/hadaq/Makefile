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

HLDPRINT_EXES     = $(HADAQDIR)/hldprint.$(SrcSuf)
HLDPRINT_EXEO     = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(HLDPRINT_EXES))
HLDPRINT_EXED     = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(HLDPRINT_EXES))

# used in the main Makefile

ALLHDRS           += $(patsubst $(HADAQDIR)/%.h, $(DABCINCPATH)/%.h, $(HADAQ_H))
ALLDEPENDENC      += $(HADAQ_D) $(HLDPRINT_EXED)

libs:: $(DABCHADAQ_LIB)

exes:: $(HLDPRINT_EXE)

clean::
	rm -f $(HLDPRINT_EXE)


##### local rules #####

$(DABCINCPATH)/%.h: $(HADAQDIR)/%.h
	@echo "Header: $@" 
	@cp -f $< $@

$(DABCHADAQ_LIB):  $(HADAQ_O) $(DABCBASE_LIB) $(DABCMBS_LIB) 
	@$(MakeLib) $(DABCHADAQ_LIBNAME) "$(HADAQ_O)" $(TGTDLLPATH) "-lDabcBase -lDabcMbs"

$(HLDPRINT_EXE) : $(HLDPRINT_EXEO) $(DABCHADAQ_LIB)
	$(LD) $(LDFLAGSPRE) -O $(HLDPRINT_EXEO) $(LIBS_CORESET) -lDabcMbs -lDabcHadaq -o $(HLDPRINT_EXE)

########### extra rules #############
ifdef hadaq-debug
$(HADAQ_O): DEFINITIONS += HADAQ_DEBUG 
endif

include $(DABCSYS)/config/Makefile.rules
