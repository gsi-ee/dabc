include $(DABCSYS)/config/Makefile.config

ifdef DABCMAINMAKE
HADAQEXAMPLEDIR   = applications/hadaq/
else
HADAQEXAMPLEDIR   = 
endif

HADAQEXAMPLE_EXE  = $(HADAQEXAMPLEDIR)example

HADAQEXAMPLE_S    = $(HADAQEXAMPLEDIR)example.$(SrcSuf)
HADAQEXAMPLE_O    = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(HADAQEXAMPLE_S))
HADAQEXAMPLE_D    = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(HADAQEXAMPLE_S))

ALLDEPENDENC += $(HADAQEXAMPLE_D)

exes::  $(HADAQEXAMPLE_EXE)

clean::
	rm -f $(HADAQEXAMPLE_EXE)

$(HADAQEXAMPLE_EXE) : $(HADAQEXAMPLE_O) $(DABCBASE_LIB) $(DABCMBS_LIB) $(DABCHADAQ_LIB)
	$(LD) -Wl,--no-as-needed -O $(HADAQEXAMPLE_O) $(LIBS_CORESET) -lDabcMbs -lDabcHadaq -o $(HADAQEXAMPLE_EXE)

include $(DABCSYS)/config/Makefile.rules
